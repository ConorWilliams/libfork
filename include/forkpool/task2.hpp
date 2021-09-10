#pragma once

#include <atomic>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#include "forkpool/detail/promise.hpp"
#include "forkpool/forkpool2.hpp"

namespace riften {

struct sync_tag {};

inline constexpr sync_tag sync() { return {}; }

inline constexpr std::uint64_t initial = (std::uint64_t(0b11) << 62);
inline constexpr std::uint64_t hi_mask = (std::uint64_t(0b10) << 62);
inline constexpr std::uint64_t lo_mask = (std::uint64_t(0b01) << 62) - 1;
inline constexpr std::uint64_t sub_bit = (std::uint64_t(0b10) << 62);

template <typename T> class [[nodiscard]] task2;
template <typename T> class [[nodiscard]] future2;

// A task2 manages a coroutine frame that is scheduled at initial suspend by the customisation point
// Scheduler. A hot task2 begins executing on the scheduler immidiatly upon creation and is scheduled via
// child stealing, hot task2s do NOT assume strict fork/join
template <typename T> class [[nodiscard]] task2 {
  public:
    class promise_type : public detail::promise_result<T> {
      private:
        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                if (!handle.promise()._continuation) {
                    // This is a root task, so we must notify its completion
                    handle.promise()._done.clear();
                    handle.promise()._done.notify_one();
                    return std::noop_coroutine();
                }

                std::uint64_t forks
                    = handle.promise()._continuation.promise()._forks.fetch_sub(1, std::memory_order_release);

                if (forks == 1) {
                    return handle.promise()._continuation;
                } else {
                    return std::noop_coroutine();
                }
            }
        };

      public:
        task2 get_return_object() noexcept {
            return task2{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // task2s are lazily started,
        std::suspend_always initial_suspend() const noexcept { return {}; }

        // and run their continuation at exit (either by co_return or exception)
        final_awaitable final_suspend() const noexcept { return {}; }

        auto await_transform(sync_tag) {
            struct awaitable {
                constexpr bool await_ready() const noexcept {
                    return _coroutine.promise()._forks.load(std::memory_order_acquire) == high;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type>) {
                    if (_coroutine.promise()._forks.fetch_sub(high, std::memory_order_acq_rel) == high) {
                        if (_coroutine.promise()._continuation) {
                            return _coroutine.promise()._continuation;
                        }
                    }
                    return std::noop_coroutine();
                }

                constexpr void await_resume() const noexcept {}

                std::coroutine_handle<promise_type> _coroutine;
            };

            return awaitable{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        template <typename A> decltype(auto) await_transform(A&& a) { return std::forward<A>(a); }

        template <typename U>
        void set_parent(std::coroutine_handle<typename task2<U>::promise_type> handle) noexcept {
            _parent = handle;
            _parent_steals = &handle.promise()._steals;
        }

        // Indicates this coroutine has either: run to final suspend or never started
        [[nodiscard]] bool done() const noexcept { return !_continuation.load(std::memory_order_acquire); }

        alignas(hardware_destructive_interference_size) std::atomic_uint64_t _steals = initial;

        std::atomic_flag _done = true;

      private:
        std::coroutine_handle<> _parent{nullptr};
        std::atomic_uint64_t* _parent_steals{nullptr};
    };

    //////////////////////////////////////////////////////////////////////////////////////

    // Initialise empty task2
    constexpr task2() : _coroutine{nullptr} {}

    // No assignment/copy constructor, task2s are 'unique'
    task2(const task2&) = delete;
    task2& operator=(task2 const&) = delete;

    // but, they can be moved.
    task2(task2&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    task2& operator=(task2&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(task2& lhs, task2& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~task2() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    decltype(auto) launch() && {
        //

        Forkpool::schedule(_coroutine);
        _coroutine.promise()._done.wait(true);
        return std::move(_coroutine.promise()).get();
    }

    auto fork() && {
        //
        struct awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) {
                //
                std::coroutine_handle<promise_type> on_stack_coro = _coroutine_of_task;

                on_stack_coro.promise().set_continuation(handle);

                try {
                    handle.promise()._forks.fetch_add(1, std::memory_order_release);
                    Forkpool::schedule(handle);
                    return on_stack_coro;
                } catch (...) {
                    // Scheduling failed therefore, we are responsible for cleaning up coroutine as we have
                    // only valid handle to _coroutine
                    destroy(on_stack_coro);
                    throw;
                }
            }

            auto await_resume() { return future2<T>(_coroutine_of_task); }

            std::coroutine_handle<promise_type> _coroutine_of_task;
        };

        if (_coroutine) {
            return awaitable{{}, std::exchange(_coroutine, nullptr)};
        } else {
            throw broken_promise{};
        }
    }

  private:
    friend class promise_type;
    // Only promise can construct a filled task2
    explicit task2(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            LOG_DEBUG("task2 .destroy() calls destroy");
            handle.destroy();
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

template <typename T> class [[nodiscard]] future2 {
  private:
    using promise_type = task2<T>::promise_type;

  public:
    // Initialise empty future2
    constexpr future2() : _coroutine{nullptr} {}

    // No assignment/copy constructor, future2s are 'unique'
    future2(const future2&) = delete;
    future2& operator=(future2 const&) = delete;

    // but, they can be moved.
    future2(future2&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    future2& operator=(future2&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(future2& lhs, future2& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~future2() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    decltype(auto) operator*() const& {
        if (_coroutine) {
            return _coroutine.promise().get();
        } else {
            throw broken_promise{};
        }
    }

    decltype(auto) operator*() && {
        //

        if (_coroutine) {
            std::move(_coroutine.promise()).get();
        } else {
            throw broken_promise{};
        }
    }

  private:
    friend class task2<T>;
    // Only task2 can construct a filled future2
    explicit future2(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            LOG_DEBUG("future2 destructor calls destroy");
            handle.destroy();
            return;
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
