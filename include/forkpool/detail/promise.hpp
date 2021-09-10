#pragma once

#include <atomic>
#include <cassert>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#include "forkpool/broken_promise.hpp"
#include "forkpool/detail/macrologger.h"

namespace riften::detail {

// General case, specialisations for void/T& to follow, provides the return_value(...) coroutine
// method and a way to access that value through .result() method
template <typename T> class promise_result {
  public:
    promise_result() noexcept {}  // Initialise empty

    void unhandled_exception() noexcept {
        assert(payload == State::empty);
        LOG_DEBUG("Stash exception");
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
                assert(false);
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
                assert(false);
            case State::result:
                return std::move(_result);
            case State::exception:
                std::rethrow_exception(_exception);
        }

        __builtin_unreachable();  // Silence g++ warnings
    }

    ~promise_result() {
        //
        LOG_DEBUG("Destruct promise.");

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

// Specialisations for void
template <> class promise_result<void> {
  public:
    promise_result() noexcept {}  // Initialise empty

    void unhandled_exception() noexcept {
        assert(!_exception);
        _exception = std::current_exception();
    }

    void return_void() const noexcept {};

    void get() const {
        if (_exception) {
            std::rethrow_exception(_exception);
        }
    }

  private:
    std::exception_ptr _exception = nullptr;  // Possible exception
};

// Maybe introduce wait/release methods
template <bool Blocking> struct binary_latch;

// EBO specialisation
template <> struct binary_latch<false> {};

// Latch is initialised 'held', wait() will block until a thread calls release().
template <> struct binary_latch<true> {
  public:
    void wait() const noexcept { _ready.wait(false, std::memory_order_acquire); }

    void release() noexcept {
        _ready.test_and_set(std::memory_order_release);
        _ready.notify_one();
    }

  private:
    std::atomic_flag _ready = false;
};

}  // namespace riften::detail
