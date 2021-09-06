#pragma once

#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "forkpool/detail/promise.hpp"

namespace riften {

template <typename T>
concept Scheduler = requires(std::coroutine_handle<> handle) {
    T::schedule(handle);
};

struct inline_scheduler {
    static constexpr bool await_ready() noexcept { return true; }
    static constexpr void schedule(std::coroutine_handle<>) {}
};

// A hot_task manages a coroutine frame that is scheduled at initial suspend by the customisation point
// Scheduler. A hot task begins executing on the scheduler immidiatly upon creation and is scheduled via child
// stealing, hot tasks do NOT assume strict fork/join
template <typename T, Scheduler Scheduler, bool Blocking = false> class [[nodiscard]] hot_task {
  public:
    class promise_type : public detail::promise_result<T>, private detail::binary_latch<Blocking> {
      private:
        struct initial_awaitable {
            constexpr bool await_ready() const noexcept {
                if constexpr (requires { Scheduler::await_ready(); }) {
                    static_assert(noexcept(Scheduler::await_ready()), "Warn if throwing");
                    return Scheduler::await_ready();
                } else {
                    return false;
                }
            }

            constexpr void await_suspend(std::coroutine_handle<promise_type> handle) const noexcept {
                try {
                    Scheduler::schedule(handle);
                } catch (...) {
                    // Scheduling failed therefore, coroutine's final_suspend will never run.
                    handle.promise().unhandled_exception();
                    handle.promise().exchange_continuation(nullptr);

                    if constexpr (Blocking) {
                        handle.promise().release();
                    }

                    // Clean-up will be handled by get_return_object()'s hot_task
                }
            }

            constexpr void await_resume() const noexcept {
                // TODO : Could cancel early if _continuation == nullptr
            }
        };

        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                // By storing nullptr we make continuation responsible for cleanup
                if (std::coroutine_handle<> tmp = handle.promise().exchange_continuation(nullptr)) {
                    // Notify anyone waiting on completion of this coroutine
                    if constexpr (Blocking) {
                        LOG_DEBUG("Release blocking get()");
                        handle.promise().release();
                    }
                    return tmp;
                } else {
                    // Task must have been destructed, therefore we are responsible for cleanup.
                    // Cannot wait on destructed hot_task therefore, no release required.
                    LOG_DEBUG("final_suspend calls destroy");
                    handle.destroy();
                    return std::noop_coroutine();
                }
            }
        };

      public:
        hot_task get_return_object() noexcept {
            return hot_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Tasks are lazily started,
        initial_awaitable initial_suspend() const noexcept { return {}; }

        // and run their continuation at exit (either by co_return or exception)
        final_awaitable final_suspend() const noexcept { return {}; }

      private:
        friend class hot_task;

        std::coroutine_handle<> exchange_continuation(std::coroutine_handle<> handle) noexcept {
            return _continuation.exchange(handle, std::memory_order_acq_rel);
        }

        // Indicates this coroutine has either: run to final suspend or never started
        bool done() const noexcept { return !_continuation.load(std::memory_order_acquire); }

        std::atomic<std::coroutine_handle<>> _continuation{std::noop_coroutine()};
    };

    //////////////////////////////////////////////////////////////////////////////////////

    constexpr hot_task() : _coroutine{nullptr} {}

    // No assignment/copy constructor, hot_tasks are 'unique'
    hot_task(const hot_task&) = delete;
    hot_task& operator=(hot_task const&) = delete;

    // but, they can be moved.
    hot_task(hot_task&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    hot_task& operator=(hot_task&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(hot_task& lhs, hot_task& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~hot_task() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    // Blocking equivilent of co_awaiting on hot_task
    decltype(auto) get() const& requires Blocking {
        if (_coroutine) {
            LOG_DEBUG("get() blocks");
            _coroutine.promise().wait();
            LOG_DEBUG("get() block is released");
            return _coroutine.promise().get();
        } else {
            throw broken_promise{};
        }
    }

    decltype(auto) get() && requires Blocking {
        if (_coroutine) {
            LOG_DEBUG("get() blocks");
            _coroutine.promise().wait();
            LOG_DEBUG("get() block is released");
            return std::move(_coroutine.promise()).get();
        } else {
            throw broken_promise{};
        }
    }

    auto operator co_await() const& {
        //
        struct awaitable : awaitable_base {
            decltype(auto) await_resume() { return this->_coroutine.promise().get(); }
        };

        if (_coroutine) {
            return awaitable{_coroutine};
        } else {
            throw broken_promise{};
        }
    }

    auto operator co_await() && {
        //
        struct awaitable : awaitable_base {
            decltype(auto) await_resume() { return std::move(this->_coroutine.promise()).get(); }
        };

        if (_coroutine) {
            return awaitable{_coroutine};
        } else {
            throw broken_promise{};
        }
    }

  private:
    friend class promise_type;
    // Only promise can construct a filled hot_task
    explicit hot_task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    struct awaitable_base {
        // Shortcut
        bool await_ready() const noexcept { return _coroutine.promise().done(); }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept {
            // Try to schedule handle as the continuation of this hot_task
            if (std::coroutine_handle<> old = _coroutine.promise().exchange_continuation(handle)) {
                // Managed to set continuation in time (got old == std::noop_coroutine()).
                LOG_DEBUG("Set continuation in-time");
                return std::noop_coroutine();
            } else {
                // While setting the continuation the hot_task completed, hence we can resume
                // the coroutine of handle.  Must re_exchange so that hot_task is in "done" state
                LOG_DEBUG("Task completed while suspending");
                _coroutine.promise().exchange_continuation(nullptr);
                return handle;
            }
        }

        std::coroutine_handle<promise_type> _coroutine;
    };

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            if (!handle.promise().exchange_continuation(nullptr)) {
                // Thread has completed hot_task, therefore we are responsible for cleanup.
                LOG_DEBUG("Task destructor calls destroy");
                handle.destroy();
                return;
            }
            // Otherwise thread will be responsible for cleanup due to nullptr we just exchanged into the
            // continuation.
            LOG_DEBUG("Task destructor does nothing");
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
