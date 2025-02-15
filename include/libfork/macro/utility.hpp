#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cassert> // for assert
#include <utility> // for unreachable
#include <version> // for __cpp_lib_unreachable, ...

/**
 * @file macro.hpp
 *
 * @brief A collection of internal/public macros.
 *
 * These are exhaustively documented due to macros nasty visibility rules
 * however, only macros that are marked as __[public]__ should be consumed.
 */

// clang-format off

/**
 * @brief Use like `BOOST_HOF_RETURNS` to define a function/lambda with all the
 * noexcept/requires/decltype specifiers.
 *
 * This macro is not truly variadic but the ``...`` allows commas in the macro argument.
 */
#define LF_HOF_RETURNS(...)                                                                        \
  noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__)                                         \
    requires requires { __VA_ARGS__; }                                                             \
  {                                                                                                \
    return __VA_ARGS__;                                                                            \
  }

// clang-format on

/**
 * @brief Macro to prevent a function to be inlined.
 */
#if !defined(LF_NOINLINE)
  #if defined(_MSC_VER) && !defined(__clang__)
    #define LF_NOINLINE __declspec(noinline)
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #if defined(__CUDACC__)
      // nvcc doesn't always parse __noinline__, see:
      // https://svn.boost.org/trac/boost/ticket/9392
      #define LF_NOINLINE __attribute__((noinline))
    #elif defined(__HIP__)
      // See https://github.com/boostorg/config/issues/392
      #define LF_NOINLINE __attribute__((noinline))
    #else
      #define LF_NOINLINE __attribute__((__noinline__))
    #endif
  #else
    #define LF_NOINLINE
  #endif
#endif

/**
 * @brief Macro to use next to 'inline' to force a function to be inlined.
 *
 * \rst
 *
 * .. note::
 *
 *    This does not imply the c++'s `inline` keyword which also has an effect on
 * linkage.
 *
 * \endrst
 */
#if !defined(LF_FORCEINLINE)
  #if defined(_MSC_VER) && !defined(__clang__)
    #define LF_FORCEINLINE __forceinline
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #define LF_FORCEINLINE __attribute__((__always_inline__))
  #else
    #define LF_FORCEINLINE
  #endif
#endif

/**
 * @brief Force no-inline for clang, works-around
 * https://github.com/llvm/llvm-project/issues/63022.
 *
 * TODO: Check __apple_build_version__ when xcode 16 is released.
 */
#if defined(__clang__)
  #if defined(__apple_build_version__) || __clang_major__ <= 16
    #define LF_CLANG_TLS_NOINLINE LF_NOINLINE
  #else
    #define LF_CLANG_TLS_NOINLINE
  #endif
#else
  #define LF_CLANG_TLS_NOINLINE
#endif
