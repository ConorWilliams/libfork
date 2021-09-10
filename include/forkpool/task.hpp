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
#include "forkpool/forkpool.hpp"

namespace riften {

template <typename T, bool Blocking = false> class [[nodiscard]] task;
template <typename T, bool Blocking = false> class [[nodiscard]] future;

template <typename T, bool Blocking> decltype(auto) launch(task<T, Blocking>&& t) {
    if constexpr (Blocking) {
        std::move(t).launch();
    } else {
        return [&]() -> task<T, true> { co_return co_await co_await std::move(t).fork(); }().launch();
    }
}

// A task manages a coroutine frame that is scheduled at initial suspend by the customisation point
// Scheduler. A hot task begins executing on the scheduler immidiatly upon creation and is scheduled via
// child stealing, hot tasks do NOT assume strict fork/join
template <typename T, bool Blocking> class [[nodiscard]] task {
  public:
    class promise_type : public detail::promise_result<T>, public detail::binary_latch<Blocking> {
      private:
        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                // By storing nullptr we make continuation responsible for cleanup
                if (std::coroutine_handle<> tmp = handle.promise().exchange_continuation(nullptr)) {
                    // Notify anyone waiting on completion of this coroutine
                    if constexpr (Blocking) {
                        handle.promise().release();
                    }
                    return tmp;
                } else {
                    // Task must have been destructed, therefore we are responsible for cleanup.
                    // Cannot wait on destructed task therefore, no release required.
                    LOG_DEBUG("final_suspend calls destroy");
                    handle.destroy();
                    return std::noop_coroutine();
                }
            }
        };

      public:
        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Tasks are lazily started,
        std::suspend_always initial_suspend() const noexcept { return {}; }

        // and run their continuation at exit (either by co_return or exception)
        final_awaitable final_suspend() const noexcept { return {}; }

        [[nodiscard]] std::coroutine_handle<> exchange_continuation(std::coroutine_handle<> handle) noexcept {
            return _continuation.exchange(handle, std::memory_order_acq_rel);
        }

        void set_continuation(std::coroutine_handle<> handle) noexcept {
            return _continuation.store(handle, std::memory_order_release);
        }

        // Indicates this coroutine has either: run to final suspend or never started
        [[nodiscard]] bool done() const noexcept { return !_continuation.load(std::memory_order_acquire); }

      private:
        std::atomic<std::coroutine_handle<>> _continuation{std::noop_coroutine()};
    };

    //////////////////////////////////////////////////////////////////////////////////////

    // Initialise empty task
    constexpr task() : _coroutine{nullptr} {}

    // No assignment/copy constructor, tasks are 'unique'
    task(const task&) = delete;
    task& operator=(task const&) = delete;

    // but, they can be moved.
    task(task&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    task& operator=(task&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(task& lhs, task& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~task() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    decltype(auto) launch() && requires Blocking {
        Forkpool::schedule(_coroutine);
        _coroutine.promise().wait();
        return std::move(_coroutine.promise()).get();
    }

    auto fork() && {
        //
        struct awaitable : std::suspend_always {
            // Shortcut
            std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
                try {
                    std::coroutine_handle<> on_stack_coro = _coroutine;
                    Forkpool::schedule(handle);
                    return on_stack_coro;
                } catch (...) {
                    // Scheduling failed therefore, we are responsible for cleaning up coroutine as we have
                    // only valid handle to _coroutine
                    destroy(_coroutine);
                    throw;
                }
            }

            auto await_resume() { return future<T, Blocking>(_coroutine); }

            std::coroutine_handle<promise_type> _coroutine;
        };

        if (_coroutine) {
            return awaitable{{}, std::exchange(_coroutine, nullptr)};
        } else {
            throw broken_promise{};
        }
    }

  private:
    friend class promise_type;
    // Only promise can construct a filled task
    explicit task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            LOG_DEBUG("Task .destroy() calls destroy");
            handle.destroy();
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

template <typename T, bool Blocking> class [[nodiscard]] future {
  private:
    using promise_type = task<T, Blocking>::promise_type;

  public:
    // Initialise empty future
    constexpr future() : _coroutine{nullptr} {}

    // No assignment/copy constructor, futures are 'unique'
    future(const future&) = delete;
    future& operator=(future const&) = delete;

    // but, they can be moved.
    future(future&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    future& operator=(future&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(future& lhs, future& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~future() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    // Blocking equivilent of co_awaiting on future
    decltype(auto) get() const& requires Blocking {
        if (_coroutine) {
            _coroutine.promise().wait();
            return _coroutine.promise().get();
        } else {
            throw broken_promise{};
        }
    }

    decltype(auto) get() && requires Blocking {
        if (_coroutine) {
            _coroutine.promise().wait();
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
    friend class task<T, Blocking>;
    // Only task can construct a filled future
    explicit future(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    struct awaitable_base {
        // Shortcut
        bool await_ready() const noexcept { return _coroutine.promise().done(); }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept {
            // Try to schedule handle as the continuation of this future
            if (std::coroutine_handle<> old = _coroutine.promise().exchange_continuation(handle)) {
                // Managed to set continuation in time (got old == std::noop_coroutine()).
                LOG_DEBUG("Set continuation in-time");
                return std::noop_coroutine();
            } else {
                // While setting the continuation the future completed, hence we can resume
                // the coroutine of handle.  Must re_set so that future is in "done" state
                LOG_DEBUG("future completed while suspending");
                _coroutine.promise().set_continuation(nullptr);
                return handle;
            }
        }

        std::coroutine_handle<promise_type> _coroutine;
    };

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            if (!handle.promise().exchange_continuation(nullptr)) {
                // Thread has completed future, therefore we are responsible for cleanup.
                LOG_DEBUG("future destructor calls destroy");
                handle.destroy();
                return;
            }
            // Otherwise thread will be responsible for cleanup due to nullptr we just exchanged into the
            // continuation.
            LOG_DEBUG("future destructor does nothing");
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
