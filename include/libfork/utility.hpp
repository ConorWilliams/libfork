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
#include <type_traits>
#include <utility>

/**
 * @file utility.hpp
 *
 * @brief A small collection of utility functions and macros.
 */

namespace lf {

/**
 * @brief libfork's error type, derived from ``std::runtime_error``.
 */
struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

#ifndef NDEBUG

namespace detail {

constexpr auto file_name(std::string_view path) -> std::string_view {
  if (auto last = path.find_last_of("/\\"); last != std::string_view::npos) {
    path.remove_prefix(last);
  }
  return path;
}

}  // namespace detail

// NOLINTBEGIN Sometime macros are the only way to do things.

/**
 * @brief Assert an expression is true and ``std::terminate()`` if not.
 */
#define ASSERT(expr, msg)                                                                       \
  do {                                                                                          \
    constexpr std::string_view fname = ::lf::detail::file_name(__FILE__);                       \
                                                                                                \
    if (!(expr)) {                                                                              \
      std::cerr << "ASSERT \"" << #expr << "\" failed in " << fname << ":" << __LINE__ << " | " \
                << (msg) << std::endl;                                                          \
      std::terminate();                                                                         \
    }                                                                                           \
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
#define ASSUME(expr) [[assume((expr))]]
#elif defined(__clang__)
#define ASSUME(expr) __builtin_assume((expr))
#elif defined(__GNUC__) && !defined(__ICC)
#define ASSUME(expr)         \
  if ((expr)) {              \
  } else {                   \
    __builtin_unreachable(); \
  }
#elif defined(_MSC_VER) || defined(__ICC)
#define ASSUME(expr) __assume((expr))
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

// NOLINTEND

namespace detail {

// Test for co_await non-member overload
template <typename T>
concept has_non_member_co_await = requires { operator co_await(std::declval<T>()); };

// Test for co_await member overload
template <typename T>
concept has_member_co_await = requires { std::declval<T>().operator co_await(); };

// Base case, for (true, true) as ambiguous overload set
template <typename T, bool Mem, bool Non>
struct get_awaiter : std::false_type {};

template <typename T>
struct get_awaiter<T, true, false> : std::true_type {
  using type = decltype(std::declval<T>().operator co_await());
};

template <typename T>
struct get_awaiter<T, false, true> : std::true_type {
  using type = decltype(operator co_await(std::declval<T>()));
};

// General case just return type
template <typename T>
struct get_awaiter<T, false, false> : std::true_type {
  using type = T;
};

// Fetch the awaiter obtained by (co_await T); either T or operator co_await
template <typename T>
using awaiter_t = typename get_awaiter<T, has_member_co_await<T>, has_non_member_co_await<T>>::type;

// Gets the return type of (co_await T).await_suspend(...)
template <typename T>
struct awaitor_suspend {
  using type = decltype(std::declval<awaiter_t<T>>().await_suspend(std::coroutine_handle<>{}));
};

template <typename T>
using suspend_t = typename awaitor_suspend<T>::type;

// Tag type
struct any {};

}  // namespace detail

/**
 * @brief Verify if a type is awaitable in a generic coroutine context.
 *
 * @tparam Result Type to verify that awaiter's ``await_resume()`` is ``std::converible_to``.
 */
template <typename T, typename Result = detail::any>
concept awaitable =
    requires {
      { std::declval<detail::awaiter_t<T>>().await_ready() } -> std::convertible_to<bool>;

      std::declval<detail::awaiter_t<T>>().await_suspend(std::coroutine_handle<>{});

      requires std::is_void_v<detail::suspend_t<T>> || std::is_same_v<detail::suspend_t<T>, bool> ||
                   std::is_convertible_v<detail::suspend_t<T>, std::coroutine_handle<>>;

      std::declval<detail::awaiter_t<T>>().await_resume();

      requires std::is_same_v<Result, detail::any> ||
                   std::convertible_to<
                       decltype(std::declval<detail::awaiter_t<T>>().await_resume()), Result>;
    };

/**
 * @brief The result type of ``co_await expr`` when ``expr`` is of type ``T``.
 */
template <awaitable T>
using await_result_t = decltype(std::declval<detail::awaiter_t<T>>().await_resume());

}  // namespace lf