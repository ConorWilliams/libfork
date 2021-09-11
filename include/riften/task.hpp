#pragma once

#include <atomic>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#include "riften/detail/promise.hpp"
#include "riften/forkpool.hpp"
#include "riften/meta.hpp"

namespace riften {

template <typename T = void> class [[nodiscard]] Task;
template <typename T = void> class [[nodiscard]] Fut;

// An awaitable type (in the context of Task<T>) that signifies a Task should wait for its children
struct [[nodiscard]] sync_tag {};
inline constexpr sync_tag sync() { return {}; }

// An awaitable type (in the context of Task<T>) that signifies a Task should post its continuation,
// garanteed to be non-null.
template <typename T> struct [[nodiscard]] fork_tag : std::coroutine_handle<typename Task<T>::promise_type> {
};

// Release coroutine owned by handle
void destroy(std::coroutine_handle<> handle) noexcept {
    if (handle) {
        LOG_DEBUG("Destroy coroutine frams");
        handle.destroy();
    }
}

// A Task manages a coroutine frame that is ...
template <typename T> class [[nodiscard]] Task {
  public:
    class promise_type : public detail::promise_result<T> {
      private:
        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> Task) const noexcept {
                //
                if (std::optional Task_handle = Forkpool::pop()) {
                    // No-one stole continuation, just keep rippin!
                    LOG_DEBUG("Keeps rippin");
                    return *Task_handle;
                }

                if (!Task.promise()._parent_n) {
                    // This must be a root Task and we must be out of Tasks
                    LOG_DEBUG("Root Task completes");
                    return std::noop_coroutine();
                }

                LOG_DEBUG("Some-one stole parent");

                // Else: register with parent we have completed this child Task
                std::uint64_t n = (Task.promise()._parent_n)->fetch_sub(1, std::memory_order_acq_rel);

                if (n == 1) {
                    // Parent has called sync and we are the last child Task to complete, hence we continue
                    // parent.
                    LOG_DEBUG("Win join race");
                    return Task.promise()._parent;
                } else {
                    // Parent has not called sync or we are not the last child to complete, so we are out of
                    // jobs, return to stealing.
                    LOG_DEBUG("Loose join race");
                    return std::noop_coroutine();
                }
            }
        };

      public:
        Task get_return_object() noexcept {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() const noexcept { return {}; }

        final_awaitable final_suspend() const noexcept { return {}; }

        // Pass regular awaitables straight through
        template <awaitable A> decltype(auto) await_transform(A&& a) { return std::forward<A>(a); }

        template <typename U> auto await_transform(fork_tag<U> child) const noexcept {
            //
            struct awaitable : std::suspend_always {
                //
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> parent) {
                    //
                    _child.promise().template set_parent<T>(parent);

                    // In-case *this (awaitable) is destructed by stealer after schedule
                    std::coroutine_handle<> on_stack_handle = _child;

                    LOG_DEBUG("Forking");

                    Forkpool::schedule({parent, std::addressof(parent.promise()._alpha)});

                    return on_stack_handle;
                }

                Fut<U> await_resume() const noexcept { return Fut<U>{_child}; }

                std::coroutine_handle<typename Task<U>::promise_type> _child;
            };

            return awaitable{{}, std::move(child)};
        }

        auto await_transform(sync_tag) noexcept {
            struct awaitable {
                constexpr bool await_ready() const noexcept {
                    if (std::uint64_t alpha = _task.promise()._alpha) {
                        if (alpha == i_max - _task.promise()._n.load(std::memory_order_acquire)) {
                            LOG_DEBUG("sync() is ready");
                            return true;
                        } else {
                            LOG_DEBUG("sync() not ready");
                            return false;
                        }
                    } else {
                        LOG_DEBUG("sync() ready (no steals)");
                        return true;
                    };
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task) noexcept {
                    // Currently        n = i_max - num_joined
                    // If we perform    n <- n - (imax - num_stolen)
                    // then             n = num_stolen - num_joined

                    std::uint64_t alpha = task.promise()._alpha;

                    std::uint64_t n = task.promise()._n.fetch_sub(i_max - alpha, std::memory_order_acq_rel);

                    if (n - (i_max - alpha) == 0) {
                        // We set n after all children had completed therefore we can resume task
                        LOG_DEBUG("sync() wins");
                        return task;
                    } else {
                        // Someone else is responsible for running this task and we have run out of work
                        LOG_DEBUG("sync() looses");
                        return std::noop_coroutine();
                    }
                }

                constexpr void await_resume() const noexcept {
                    // After a sync we reset alpha/n
                    _task.promise()._alpha = 0;
                    _task.promise()._n.store(i_max);
                }

                std::coroutine_handle<promise_type> _task;
            };

            // Check if num-joined == num-stolen
            return awaitable{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::atomic_uint64_t* get_n() noexcept { return std::addressof(_n); }

        template <typename U>
        void set_parent(std::coroutine_handle<typename Task<U>::promise_type> parent) noexcept {
            _parent = parent;
            _parent_n = parent.promise().get_n();
        }

      private:
        std::coroutine_handle<> _parent = nullptr;
        std::atomic_uint64_t* _parent_n = nullptr;

        std::uint64_t _alpha = 0;

        alignas(hardware_destructive_interference_size) std::atomic_uint64_t _n = i_max;

        static constexpr std::uint64_t i_max = std::numeric_limits<std::uint64_t>::max();

        friend class Task<T>;
    };

    //////////////////////////////////////////////////////////////////////////////////////

    // Initialise empty Task
    constexpr Task() : _coroutine{nullptr} {}

    // No assignment/copy constructor, Tasks are 'unique'
    Task(const Task&) = delete;
    Task& operator=(Task const&) = delete;

    // but, they can be moved.
    Task(Task&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    decltype(auto) launch() && {
        Forkpool::schedule_root({_coroutine, std::addressof(_coroutine.promise()._alpha)});
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    auto fork() && {
        if (_coroutine) {
            return fork_tag<T>{std::exchange(_coroutine, nullptr)};
        } else {
            throw broken_promise{};
        }
    }

    ~Task() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

  private:
    friend class promise_type;
    // Only promise can construct a filled Task
    explicit Task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    std::coroutine_handle<promise_type> _coroutine;
};

template <typename T> class [[nodiscard]] Fut {
  private:
    using promise_type = Task<T>::promise_type;

  public:
    // Initialise empty Fut
    constexpr Fut() : _coroutine{nullptr} {}

    explicit Fut(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // No assignment/copy constructor, Futs are 'unique'
    Fut(const Fut&) = delete;
    Fut& operator=(Fut const&) = delete;

    // but, they can be moved.
    Fut(Fut&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    Fut& operator=(Fut&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

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

    ~Fut() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

  private:
    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
