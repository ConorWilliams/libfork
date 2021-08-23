#pragma once

#include <atomic>
#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "coop/broken_promise.hpp"

namespace riften {

template <typename T = void> class [[nodiscard]] task;

namespace detail {

// Coroutine-promise exception handling, provides unhandled_exception()
class exception_base {
  public:
    void unhandled_exception() noexcept { _exception = std::current_exception(); }

    void rethrow_if_unhandled_exception() const {
        if (_exception) {
            std::rethrow_exception(_exception);
        }
    }

    bool exception_ready() const noexcept { return static_cast<bool>(_exception); }

  private:
    std::exception_ptr _exception = nullptr;
};

// General case, specialisations for void/T& to follow, provides the return_value(...) coroutine
// method and a way to access that value through .result() method
template <typename T> class promise_base : public exception_base {
  public:
    promise_base() noexcept : _empty() {}  // Initialise empty

    template <typename U> void return_value(U&& expr) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        std::construct_at(std::addressof(_result), std::forward<U>(expr));
    }

    T const& result() const& {
        exception_base::rethrow_if_unhandled_exception();
        return _result;
    }

    T result() && {
        exception_base::rethrow_if_unhandled_exception();
        return std::move(_result);
    }

    // Explicit default for trivial destructibility
    ~promise_base() requires std::is_trivially_destructible<T>::value = default;

    ~promise_base() requires std::negation<std::is_trivially_destructible<T>>::value {
        if (!exception_base::exception_ready()) {
            std::destroy_at(std::addressof(_result));
        }
    }

  private : struct empty_byte {};
    union {
        empty_byte _empty;  // In case T has a non-trivial default constructor
        T _result;
    };
};

// // Specialisations for T&
// template <typename T> class promise_base<T&> : public exception_base {
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
// template <> class promise_base<void> : public exception_base {
//   public:
//     void return_void() const noexcept {};

//     void result() const { rethrow_if_unhandled_exception(); };
// };

}  // namespace detail

template <typename T> class task {
  public:
    struct promise_type : detail::promise_base<T> {
      private:
        // struct initial_awaitable : std::suspend_always {
        //     auto await_suspend(std::coroutine_handle<promise> handle) const { return
        //     Scheduler::schedule(handle); }
        // };

        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                if (std::coroutine_handle<> tmp = handle.promise().exchange_continuation(nullptr)) {
                    // Run continuation, by storing nullptr we make continuation responsible for cleanup
                    return tmp;
                } else {
                    // Task must have been destructed, therefore we are responsible for cleanup
                    handle.destroy();
                }

                return std::noop_coroutine();
            }
        };

      public:
        task get_return_object() noexcept {
            return task<T>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Tasks are lazily started,
        std::suspend_always initial_suspend() const noexcept { return {}; }

        // and run their continuation at exit (either by co_return or exception)
        final_awaitable final_suspend() const noexcept { return {}; }

        std::coroutine_handle<> exchange_continuation(std::coroutine_handle<> handle) noexcept {
            return _continuation.exchange(handle, std::memory_order_acq_rel);
        }

        // Indicates this coroutine has been run to final_suspend or task destructed.
        bool done() const noexcept { return !_continuation.load(std::memory_order_acquire); }

      private:
        static_assert(std::atomic<std::coroutine_handle<>>::is_always_lock_free, "");

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

    // Schedule this coroutine for resumption after task is completed.
    auto operator co_await() && {
        //
        struct awaitable : awaitable_base {
            decltype(auto) await_resume() { return std::move(this->_coroutine.promise()).result(); }
        };

        if (_coroutine) {
            return awaitable{_coroutine};
        } else {
            throw broken_promise{};
        }
    }

    auto operator co_await() const& {
        //
        struct awaitable : awaitable_base {
            decltype(auto) await_resume() { return this->_coroutine.promise().result(); }
        };

        if (_coroutine) {
            return awaitable{_coroutine};
        } else {
            throw broken_promise{};
        }
    }

    ~task() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

  private:
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
                // the coroutine of handle.  Must re_exchange so that task is in "done" state
                return _coroutine.promise().exchange_continuation(nullptr);
            }
        }

        std::coroutine_handle<promise_type> _coroutine;
    };

    friend struct promise_type;
    // Only promise can construct a filled task
    explicit task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

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
