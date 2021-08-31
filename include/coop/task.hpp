#pragma once

#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <semaphore>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "coop/exception.hpp"
#include "coop/meta.hpp"

namespace riften {

template <typename T>
concept Scheduler = requires(std::coroutine_handle<> handle) {
    T::schedule(handle);
};

struct inline_scheduler {
    static constexpr bool await_ready() noexcept { return true; }

    static void schedule(std::coroutine_handle<>) {}
};

template <typename T, Scheduler Scheduler = inline_scheduler, bool Blocking = false> class [[nodiscard]] task;

namespace detail {

// General case, specialisations for void/T& to follow, provides the return_value(...) coroutine
// method and a way to access that value through .result() method
template <typename T> class promise_result {
  public:
    promise_result() noexcept {}  // Initialise empty

    void unhandled_exception() noexcept {
        assert(payload == State::empty);
        _exception = std::current_exception();
        payload = State::exception;
    }

    template <typename U> void return_value(U&& expr) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        assert(payload == State::empty);
        std::construct_at(std::addressof(_result), std::forward<U>(expr));
        payload = State::result;
    }

    T const& get() const& {
        switch (payload) {
            case State::empty:
                throw empty_promise{};
            case State::result:
                return _result;
            case State::exception:
                std::rethrow_exception(_exception);
        }

        __builtin_unreachable();  // Silence g++ warnings
    }

    T get() && {
        switch (payload) {
            case State::empty:
                throw empty_promise{};
            case State::result:
                return std::move(_result);
            case State::exception:
                std::rethrow_exception(_exception);
        }

        __builtin_unreachable();  // Silence g++ warnings
    }

    ~promise_result() {
        switch (payload) {
            case State::empty:
                return;
            case State::exception:
                std::destroy_at(std::addressof(_exception));
                return;
            case State::result:
                std::destroy_at(std::addressof(_result));
                return;
        }
    }

  private:
    enum class State { empty, exception, result };

    union {
        std::exception_ptr _exception = nullptr;  // Possible exception
        T _result;                                // Result space
    };

    State payload = State::empty;
};

// // Specialisations for T&
// template <typename T> class promise_result<T&> : public exception_base {
//   public:
//     void return_value(T& result) noexcept { _result = std::addressof(result); }

//     T const& result() const& {
//         exception_base::rethrow_if_unhandled_exception();
//         return *_result;
//     }

//     T&& result() && {
//         exception_base::rethrow_if_unhandled_exception();
//         return std::move(*_result);
//     }

//   private:
//     T* _result;
// };

// // Specialisations for void
// template <> class promise_result<void> : public exception_base {
//   public:
//     void return_void() const noexcept {};

//     void result() const { rethrow_if_unhandled_exception(); };
// };

// Maybe introduce wait/release methods
template <bool Blocking> struct binary_latch;

// EBO specialisation
template <> struct binary_latch<false> {};

// Latch is initialised 'held', wait() will block until a thread calls release().
template <> struct binary_latch<true> {
  public:
    void wait() const noexcept { ready.wait(false, std::memory_order_acquire); }

    void release() noexcept {
        ready.test_and_set(std::memory_order_release);
        ready.notify_one();
    }

  private:
    std::atomic_flag ready = false;
};

}  // namespace detail

template <typename T, Scheduler Scheduler, bool Blocking> class task {
  public:
    struct promise_type : public detail::promise_result<T>, private detail::binary_latch<Blocking> {
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
                }
            }

            constexpr void await_resume() const noexcept {}
        };

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
        initial_awaitable initial_suspend() const noexcept { return {}; }

        // and run their continuation at exit (either by co_return or exception)
        final_awaitable final_suspend() const noexcept { return {}; }

      private:
        friend class task;

        std::coroutine_handle<> exchange_continuation(std::coroutine_handle<> handle) noexcept {
            return _continuation.exchange(handle, std::memory_order_acq_rel);
        }

        // Indicates this coroutine has either: run to final suspend or never started
        bool done() const noexcept { return !_continuation.load(std::memory_order_acquire); }

        std::atomic<std::coroutine_handle<>> _continuation{std::noop_coroutine()};
    };

    //////////////////////////////////////////////////////////////////////////////////////

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

    // Blocking equivilent of co_awaiting on task
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
    friend struct promise_type;
    // Only promise can construct a filled task
    explicit task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    struct awaitable_base {
        // Shortcut
        bool await_ready() const noexcept { return _coroutine.promise().done(); }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept {
            // Try to schedule handle as the continuation of this task
            if (std::coroutine_handle<> old = _coroutine.promise().exchange_continuation(handle)) {
                // Managed to set continuation in time (got old == std::noop_coroutine()).
                return std::noop_coroutine();
            } else {
                // While setting the continuation the task completed, hence we can resume
                // the coroutine of handle.  Must re_exchange so that task is in "done" payload
                return _coroutine.promise().exchange_continuation(nullptr);
            }
        }

        std::coroutine_handle<promise_type> _coroutine;
    };

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            if (!handle.promise().exchange_continuation(nullptr)) {
                // Thread has completed task, therefore we are responsible for cleanup.
                handle.destroy();
            }
            // Otherwise thread will be responsible for cleanup due to nullptr we just exchanged into the
            // continuation.
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
