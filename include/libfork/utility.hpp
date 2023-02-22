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

// NOLINTBEGIN Sometime macros are the only way to do things.

#if defined(__cpp_lib_source_location)

  #include <source_location>

namespace lf::detail {
using source_location = std::source_location;
}  // namespace lf::detail

#elif defined(__cpp_lib_experimental_source_location)

  #include <experimental/source_location>

namespace lf::detail {
using source_location = std::experimental::source_location;
}  // namespace lf::detail

#else

namespace lf::detail {
struct source_location {
  static constexpr source_location current() noexcept { return source_location{}; }
  constexpr char const* file_name() const noexcept { return "unknowm"; }
  constexpr std::uint_least32_t line() const noexcept { return 0; }
  constexpr std::uint_least32_t column() const noexcept { return 0; }
  constexpr char const* function_name() const noexcept { return "unknowm"; }
};
}  // namespace lf::detail

#endif

namespace lf {

namespace detail {

constexpr auto file_name(std::string_view path) -> std::string_view {
  if (auto last = path.find_last_of("/\\"); last != std::string_view::npos) {
    path.remove_prefix(last);
  }
  return path;
}

#ifndef NDEBUG

inline void assert_impl(std::string_view const expr, std::string_view const message, source_location const location = source_location::current()) {
  std::osyncstream synced_out(std::cerr);

  synced_out << "\033[1;31mERR\033[0m: [";
  synced_out << std::this_thread::get_id();
  synced_out << "] ";
  synced_out << ::lf::detail::file_name(location.file_name());
  synced_out << "(";
  synced_out << location.line();
  synced_out << ":";
  synced_out << location.column();
  synced_out << ") \033[1;31mASSERT\033[0m: \"";
  synced_out << expr;
  synced_out << "\" failed with message: \"";
  synced_out << message;
  synced_out << "\"\n";
}

  /**
   * @brief Assert an expression is true and ``std::terminate()`` if not, a no-op if ``NDEBUG`` is defined.
   */
  #define ASSERT(expr, message)                                                                            \
    do {                                                                                                   \
      if (!(expr)) {                                                                                       \
        if (!std::is_constant_evaluated()) {                                                               \
          ::lf::detail::assert_impl(#expr, message); /* Indirection as ``std::osyncstream`` is virtual. */ \
        }                                                                                                  \
        std::terminate();                                                                                  \
      }                                                                                                    \
    } while (false)

#else
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
 *    than ``[[assume(expr)]]`` as it WILL evaluate the exprssion at runtime. Hence you should
 *    conservativly only use this macro if ``expr`` is side-effect free and cheap to evaluate.
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

#ifndef NDEBUG
/**
 * @brief `ASSUME()`` if ``NDEBUG`` is defined, otherwise ``ASSERT()``.
 *
 * Only use if ``expr`` is side-effect free and cheap to evaluate as it MAY be evaluated at runtime.
 */
  #define ASSERT_ASSUME(expr, message) ASSERT(expr, message)
#else
  #define ASSERT_ASSUME(expr, message) ASSUME(expr)
#endif

#if !defined(NDEBUG) || !defined(FORK_NO_LOGGING)

inline void log_impl(std::string_view const message, source_location const location = source_location::current()) {
  std::osyncstream synced_out(std::clog);

  synced_out << "\033[1;32mLOG\033[0m: [";
  synced_out << std::this_thread::get_id();
  synced_out << "] ";
  synced_out << file_name(location.file_name());
  synced_out << "(";
  synced_out << location.line();
  synced_out << ":";
  synced_out << location.column();
  synced_out << ") \"";
  synced_out << message;
  synced_out << "\"\n";
}

  /**
   * @brief Log a message to ``std::clog``, a no-op if ``FORK_NO_LOGGING`` or ``NDEBUG`` is defined.
   */
  #define DEBUG_TRACKER(message)                                                             \
    if (!std::is_constant_evaluated()) {                                                     \
      ::lf::detail::log_impl(message); /* Indirection as ``std::osyncstream`` is virtual. */ \
    }
#else
  #define DEBUG_TRACKER(message) \
    do {                         \
    } while (false)
#endif

// NOLINTEND

}  // namespace detail

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
 * .. include:: ../../test/glob/utility.cpp
 *    :code:
 *    :start-after: // !BEGIN-EXAMPLE
 *    :end-before: // !END-EXAMPLE
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
  constexpr defer(F && to_defer) : m_f(std::forward<F>(to_defer)) {}  // NOLINT

  defer(defer const&) = delete;
  defer(defer && other) = delete;
  auto operator=(defer const&)->defer& = delete;
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

/**
 * @brief Deduction guide for ``defer``.
 */
template <typename F>
defer(F&&) -> defer<F>;

}  // namespace lf