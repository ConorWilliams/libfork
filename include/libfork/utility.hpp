#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>
#include <exception>
#include <functional>
#include <iostream>
#include <string_view>
#include <syncstream>
#include <thread>
#include <type_traits>
#include <utility>
#include <version>

/**
 * @file utility.hpp
 *
 * @brief A small collection of utility functions and macros.
 */

namespace lf::detail {

constexpr auto file_name(std::string_view path) -> std::string_view {
  if (auto last = path.find_last_of("/\\"); last != std::string_view::npos) {
    path.remove_prefix(last);
  }
  return path;
}

}  // namespace lf::detail

// NOLINTBEGIN Sometime macros are the only way to do things.

#ifndef NDEBUG

#include <chrono>

/**
 * @brief Assert an expression is true and ``std::terminate()`` if not.
 */
#define ASSERT(expr, message)                                           \
  do {                                                                  \
    constexpr auto location = ::lf::detail::source_location::current(); \
                                                                        \
    if (!(expr)) {                                                      \
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));     \
      std::cerr << "\033[1;31mERR\033[0m: [";                           \
      std::cerr << std::this_thread::get_id();                          \
      std::cerr << "] ";                                                \
      std::cerr << ::lf::detail::file_name(location.file_name());       \
      std::cerr << "(";                                                 \
      std::cerr << location.line();                                     \
      std::cerr << ":";                                                 \
      std::cerr << location.column();                                   \
      std::cerr << ") ASSERT \"";                                       \
      std::cerr << #expr;                                               \
      std::cerr << "\" failed with message: \"";                        \
      std::cerr << message;                                             \
      std::cerr << "\"\n";                                              \
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));     \
      std::terminate();                                                 \
    }                                                                   \
  } while (false)

#else

/**
 * @brief Assert an expression is true and ``std::terminate()`` if not.
 */
#define ASSERT(...) \
  do {              \
  } while (false)

#endif  // !NDEBUG

/**
 * @brief A wrapper for C++23's ``[[assume(expr)]]`` attribute.
 *
 * Reverts to compiler specific implementations if the attribute is not available.
 *
 * \rst
 *
 *  .. warning::
 *
 *    Using some intrinsics (i.e. GCC's ``__builtin_unreachable()``) this has different semantics
 *    than ``[[assume(expr)]]`` as it WILL evaluate the exprssion at runtime.
 *
 * \endrst
 */
#if __has_cpp_attribute(assume)
#define ASSUME(expr) [[assume(bool(expr))]]
#elif defined(__clang__)
#define ASSUME(expr) __builtin_assume(bool(expr))
#elif defined(__GNUC__) && !defined(__ICC)
#define ASSUME(expr)         \
  if (bool(expr)) {          \
  } else {                   \
    __builtin_unreachable(); \
  }
#elif defined(_MSC_VER) || defined(__ICC)
#define ASSUME(expr) __assume(bool(expr))
#else
#warning "No ASSUME() implementation for this compiler."
#define ASSUME(expr) \
  do {               \
  } while (false)
#endif

/**
 * @brief ``ASSERT()`` in debug builds, ``ASSUME()`` in release builds.
 */
#ifndef NDEBUG
#define CHECK_ASSUME(expr) ASSERT(expr, "Assumption failed.")
#else
#define CHECK_ASSUME(expr) ASSUME(expr)
#endif

/**
 * @brief ``ASSERT()`` in debug builds, ``ASSUME()`` in release builds.
 *
 * Only use if ``expr`` is cheap to evaluate as it MAY be evaluated at runtime.
 */
#ifndef NDEBUG
#define ASSERT_ASSUME(expr, message) ASSERT(expr, message)
#else
#define ASSERT_ASSUME(expr, message) ASSUME(expr)
#endif

/**
 * @brief Log a message to ``std::cerr``.
 */
#ifndef NLOG

#if defined(__cpp_lib_source_location)

#include <source_location>
namespace lf::detail {
/**
 * @brief An alias for ``std::source_location``.
 */
using source_location = std::source_location;
}  // namespace lf::detail

#elif defined(__cpp_lib_experimental_source_location)

#include <experimental/source_location>

namespace lf::detail {
/**
 * @brief  An alias for ``std::experimental::source_location``.
 */
using source_location = std::experimental::source_location;
}  // namespace lf::detail

#else

namespace lf::detail {
/**
 * @brief A minimal implementation of ``std::source_location``.
 */
struct source_location {
  static constexpr source_location current() noexcept { return source_location{}; }
  constexpr const char* file_name() const noexcept { return "unknowm"; }
  constexpr std::uint_least32_t line() const noexcept { return 0; }
  constexpr std::uint_least32_t column() const noexcept { return 0; }
  constexpr const char* function_name() const noexcept { return "unknowm"; }
};
}  // namespace lf::detail

#endif

namespace lf {

namespace detail {

inline void log_impl(std::string_view const message, detail::source_location const location) {
  std::osyncstream synced_out(std::clog);

  synced_out << "\033[1;32mLOG\033[0m: [";
  synced_out << std::this_thread::get_id();
  synced_out << "] ";
  synced_out << detail::file_name(location.file_name());
  synced_out << "(";
  synced_out << location.line();
  synced_out << ":";
  synced_out << location.column();
  synced_out << ") \"";
  synced_out << message;
  synced_out << "\"\n";
}
}  // namespace detail

/**
 * @brief Log a message to ``std::clog``.
 *
 * Does nothing in constant expressions.
 */
inline constexpr void log(std::string_view const message, detail::source_location const location) {
  if (!std::is_constant_evaluated()) {
    detail::log_impl(message, location);  // Indirection as ``std::osyncstream`` is virtual.
  }
}

}  // namespace lf

/**
 * @brief Log a message to ``std::clog``.
 */
#define DEBUG_TRACKER(message) ::lf::log((message), ::lf::detail::source_location::current())
#else
#define DEBUG_TRACKER(message) \
  do {                         \
  } while (false)
#endif

// NOLINTEND

namespace lf {

/**
 * @brief libfork's error type, derived from ``std::runtime_error``.
 */
struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace detail {

// Test for co_await member overload.
template <typename T>
concept has_member_co_await = requires { std::declval<T>().operator co_await(); };

// Test for co_await non-member overload.
template <typename T>
concept has_non_member_co_await = requires { operator co_await(std::declval<T>()); };

// Test for both co_awit overloads.
template <typename T>
concept has_both_co_await = has_non_member_co_await<T> && has_member_co_await<T>;

template <typename T>
struct awaiter : std::type_identity<T> {};

template <has_member_co_await T>
struct awaiter<T> : std::type_identity<decltype(std::declval<T>().operator co_await())> {};

template <has_non_member_co_await T>
struct awaiter<T> : std::type_identity<decltype(operator co_await(std::declval<T>()))> {};

template <has_both_co_await T>
struct awaiter<T> {};

// Fetch the awaiter obtained by (co_await T).
template <typename T>
using awaiter_t = typename awaiter<T>::type;

// Tag type.
struct any {};

template <typename T>
concept void_bool_or_coro = std::is_void_v<T> || std::is_same_v<T, bool> || std::is_convertible_v<T, std::coroutine_handle<>>;

template <typename T, typename R>
concept is_same_or_any = std::is_same_v<R, any> || std::convertible_to<T, R>;

}  // namespace detail

// clang-format off

/**
 * @brief Verify if a type is awaitable in a generic coroutine context.
 *
 * @tparam Result Type to verify that awaiter's ``await_resume()`` is ``std::converible_to``.
 */
template <typename T, typename Result = detail::any>
concept awaitable = requires(std::coroutine_handle<> handle) {
  { std::declval<detail::awaiter_t<T>>().await_ready() } -> std::convertible_to<bool>;
  { std::declval<detail::awaiter_t<T>>().await_suspend(handle) } -> detail::void_bool_or_coro;
  { std::declval<detail::awaiter_t<T>>().await_resume() } -> detail::is_same_or_any<Result>;
};

// clang-format on

/**
 * @brief The result type of ``co_await expr`` when ``expr`` is of type ``T``.
 */
template <awaitable T>
using await_result_t = decltype(std::declval<detail::awaiter_t<T>>().await_resume());

/**
 * @brief Basic implementation of a Golang like defer.
 *
 *
 * \tparam F The nullary invocable's type, this **MUST** be deducted through CTAD by the deduction
 * guide and it must be ``noexcept`` callable.
 *
 * \rst
 *
 * Example:
 *
 * .. include:: ../../examples/utility/defer.cpp
 *    :code:
 *
 * \endrst
 */
template <class F>
requires std::is_nothrow_invocable_v<F&&>
class [[nodiscard("use a regular function")]] defer {
 public:
  /**
   * @brief Defer a nullary invocable until destruction of this object.
   *
   * @param to_defer Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr explicit defer(F && to_defer) : m_f(std::forward<F>(to_defer)) {}

  defer(const defer&) = delete;
  defer(defer && other) = delete;
  auto operator=(const defer&)->defer& = delete;
  auto operator=(defer&&)->defer& = delete;

  /**
   * @brief Calls the invocable.
   */
  ~defer() noexcept {
    std::invoke(std::forward<F>(m_f));
  }

 private:
  F m_f;
};

}  // namespace lf