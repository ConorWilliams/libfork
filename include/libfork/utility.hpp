#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>
#include <iostream>
#include <string_view>

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

}  // namespace lf