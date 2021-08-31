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

#include "coop/broken_promise.hpp"

namespace riften {

template <typename T = void, bool Blocking = false> class [[nodiscard]] task;

namespace detail {

// General case, specialisations for void/T& to follow, provides the return_value(...) coroutine
// method and a way to access that value through .result() method
template <typename T> class promise_base {
  public:
    promise_base() noexcept {}  // Initialise empty

    void unhandled_exception() noexcept {
        assert(state == empty);
        _exception = std::current_exception();
        state = exception;
    }

    template <typename U> void return_value(U&& expr) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        assert(state == empty);
        std::construct_at(std::addressof(_result), std::forward<U>(expr));
        state = result;
    }

    T const& get() const& {
        switch (state) {
            case empty:
                throw empty_promise{};
            case result:
                return _result;
            case exception:
                std::rethrow_exception(_exception);
        }
    }

    T get() && {
        switch (state) {
            case empty:
                throw empty_promise{};
            case result:
                return std::move(_result);
            case exception:
                std::rethrow_exception(_exception);
        }
    }

    ~promise_base() {
        switch (state) {
            case empty:
                return;
            case exception:
                std::destroy_at(std::addressof(_exception));
                return;
            case result:
                std::destroy_at(std::addressof(_result));
                return;
        }
    }

  private:
    enum State { empty, exception, result };

    union {
        std::exception_ptr _exception = nullptr;  // Possible exception
        T _result;                                // Result space
    };

    State state = empty;
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

template <bool Blocking> struct latch;

template <> struct latch<true> { std::atomic_flag ready = false; };

template <> struct latch<false> {};

}  // namespace detail

// A task packages a promise and and some work to execute
template <typename T, bool Blocking> class task {
  public:
    struct promise_type : detail::promise_base<T>, private detail::latch<Blocking> {
      private:
        struct final_awaitable;

      public:
        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Tasks are lazily started,
        std::suspend_always initial_suspend() const noexcept { return {}; }

        // and run their continuation at exit (either by co_return or exception)
        final_awaitable final_suspend() const noexcept { return {}; }

      private:
        friend class task;

        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                // By storing nullptr we make continuation responsible for cleanup
                if (std::coroutine_handle<> tmp = handle.promise().exchange_continuation(nullptr)) {
                    // Notify anyone waiting on completion of this coroutine
                    if constexpr (Blocking) {
                        handle.promise().ready.test_and_set(std::memory_order_release);
                        handle.promise().ready.notify_one();
                    }
                    return tmp;
                } else {
                    // Task must have been destructed, therefore we are responsible for cleanup
                    handle.destroy();
                }

                return std::noop_coroutine();
            }
        };

        std::coroutine_handle<> exchange_continuation(std::coroutine_handle<> handle) noexcept {
            return _continuation.exchange(handle, std::memory_order_acq_rel);
        }

        // Indicates this coroutine has either: run to final suspend or never started
        bool done() const noexcept { return !_continuation.load(std::memory_order_acquire); }

        void wait() requires Blocking { this->ready.wait(false, std::memory_order_acquire); }

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

    // The caller promises to eventually call .resume() exactly once on the returned coroutine handle if this
    // function returns without exception.
    std::pair<future<T, Blocking>, std::coroutine_handle<promise_type>> start() {
        if (_coroutine) {
            return _coroutine;
        } else {
            throw broken_promise{};
        }
    }

  private:
    friend struct promise_type;
    // Only promise can construct a filled task
    explicit task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    std::coroutine_handle<promise_type> _coroutine;
};

// A future packages a promise and and some work to execute
template <typename T, bool Blocking> class future {
  public:
    // No assignment/copy constructor, tasks are 'unique'
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

    ~future() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

  private:
    using promise_type = task<T, Blocking>::promise_type;

    struct awaitable_base {
        // Shortcut
        bool await_ready() const noexcept { return _coroutine.promise().done(); }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept {
            // Try to schedule handle as the continuation of this future
            if (std::coroutine_handle<> old = _coroutine.promise().exchange_continuation(handle)) {
                // Managed to set continuation in time (got old == std::noop_coroutine()).
                return std::noop_coroutine();
            } else {
                // While setting the continuation the future completed, hence we can resume
                // the coroutine of handle.  Must re_exchange so that future is in "done" state
                return _coroutine.promise().exchange_continuation(nullptr);
            }
        }

        std::coroutine_handle<promise_type> _coroutine;
    };

    friend class task<T, Blocking>;
    // Only promise can construct a filled future
    explicit future(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            if (!handle.promise().exchange_continuation(nullptr)) {
                // Thread has completed future, therefore we are responsible for cleanup.
                std::cout << "destroy\n";
                handle.destroy();
            } else {
                std::cout << "no destroy\n";
            }
            // Otherwise thread will be responsible for cleanup due to nullptr we just exchanged into the
            // continuation.
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
