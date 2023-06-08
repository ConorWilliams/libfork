#ifndef C5DCA647_8269_46C2_B76F_5FA68738AEDA
#define C5DCA647_8269_46C2_B76F_5FA68738AEDA

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <version>

/**
 * @file macro.hpp
 *
 * @brief A collection of internal macros + configs.
 */

// NOLINTBEGIN Sometime macros are the only way to do things...

namespace lf::detail {

/**
 * @brief The cache line size of the current architecture.
 */
#ifdef __cpp_lib_hardware_interference_size
inline constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t k_cache_line = 64;
#endif

} // namespace lf::detail

/**
 * @brief Detects if the compiler has exceptions enabled.
 */
#if defined(__cpp_exceptions) || (defined(_MSC_VER) && defined(_CPPUNWIND)) || defined(__EXCEPTIONS)
  #define LIBFORK_COMPILER_EXCEPTIONS 1
#else
  #define LIBFORK_COMPILER_EXCEPTIONS 0
#endif

/**
 * @brief If truthy then coroutines propagate exceptions, if false then termination is triggered.
 */
#ifndef LIBFORK_PROPAGATE_EXCEPTIONS
  #define LIBFORK_PROPAGATE_EXCEPTIONS LIBFORK_COMPILER_EXCEPTIONS
#endif

#if !LIBFORK_COMPILER_EXCEPTIONS && LIBFORK_PROPAGATE_EXCEPTIONS
  #error "Cannot propagate exceptions without exceptions enabled!"
#endif

/**
 * @brief A wrapper for C++23's ``[[assume(expr)]]`` attribute.
 *
 * Reverts to compiler specific implementations if the attribute is not
 * available.
 *
 * \rst
 *
 *  .. warning::
 *
 *    Using some intrinsics (i.e. GCC's ``__builtin_unreachable()``) this has
 *    different semantics than ``[[assume(expr)]]`` as it WILL evaluate the
 *    exprssion at runtime. Hence you should conservativly only use this macro
 *    if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */
#if __has_cpp_attribute(assume)
  #define LIBFORK_ASSUME(expr) [[assume(bool(expr))]]
#elif defined(__clang__)
  #define LIBFORK_ASSUME(expr) __builtin_assume(bool(expr))
#elif defined(__GNUC__) && !defined(__ICC)
  #define LIBFORK_ASSUME(expr) \
    if (bool(expr)) {          \
    } else {                   \
      __builtin_unreachable(); \
    }
#elif defined(_MSC_VER) || defined(__ICC)
  #define LIBFORK_ASSUME(expr) __assume(bool(expr))
#else
  #warning "No LIBFORK_ASSUME() implementation for this compiler."
  #define LIBFORK_ASSUME(expr) \
    do {                       \
    } while (false)
#endif

/**
 * @brief If NDEBUG is defined then LIBFORK_ASSERT(x) is  LIBFORK_ASSUME(x) otherwise assert(x)
 */
#ifndef NDEBUG
  #include <cassert>
  #define LIBFORK_ASSERT(expr) assert(expr)
#else
  #define LIBFORK_ASSERT(expr) LIBFORK_ASSUME(expr)
#endif

/**
 * @brief A customisable logging macro.
 */
#ifndef LIBFORK_LOG
  #ifndef LIBFORK_NO_LOGGING
    #include <iostream>
    #include <type_traits>
    #define LIBFORK_LOG(message, ...)        \
      do {                                   \
        if (!std::is_constant_evaluated()) { \
          std::cout << message << '\n';      \
        }                                    \
      } while (false)
  #else
    #define LIBFORK_LOG(head, ...) \
      do {                         \
      } while (false)
  #endif
#endif

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */
