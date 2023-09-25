#ifndef C5DCA647_8269_46C2_B76F_5FA68738AEDA
#define C5DCA647_8269_46C2_B76F_5FA68738AEDA

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include <version>

/**
 * @file macro.hpp
 *
 * @brief A collection of internal/public macros.
 *
 * These are exhaustively documented due to macros nasty visibility rules however, only
 * macros that are marked as __[public]__ should be consumed.
 */

// NOLINTBEGIN Sometime macros are the only way to do things...

/**
 * @brief __[public]__ The major version of libfork.
 *
 * Changes with incompatible API changes.
 */
#define LF_VERSION_MAJOR 3
/**
 * @brief __[public]__ The minor version of libfork.
 *
 * Changes when functionality is added in an API backward compatible manner.
 */
#define LF_VERSION_MINOR 2
/**
 * @brief __[public]__ The patch version of libfork.
 *
 * Changes when bug fixes are made in an API backward compatible manner.
 */
#define LF_VERSION_PATCH 0

#ifndef LF_ASYNC_STACK_SIZE
  /**
   * @brief __[public]__ A customizable stack size for ``async_stack``'s (in multiples of 4 kibibytes i.e. the page size).
   *
   * You can override this by defining ``LF_ASYNC_STACK_SIZE`` to a power of two.
   */
  #define LF_ASYNC_STACK_SIZE 256
#endif

static_assert(LF_ASYNC_STACK_SIZE && !(LF_ASYNC_STACK_SIZE & (LF_ASYNC_STACK_SIZE - 1)), "Must be a power of 2");

/**
 * @brief Use to conditionally decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) with ``static``.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CALL static
#else
  #define LF_STATIC_CALL
#endif

/**
 * @brief Use with ``LF_STATIC_CALL`` to conditionally decorate ``operator()`` with ``const``.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CONST
#else
  #define LF_STATIC_CONST const
#endif

// clang-format off

/**
 * @brief Use like `BOOST_HOF_RETURNS` to define a function/lambda with all the noexcept/requires/decltype specifiers.
 * 
 * This macro is not truly variadic but the ``...`` allows commas in the macro argument.
 */
#define LF_HOF_RETURNS(...) noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) requires requires { __VA_ARGS__; } { return __VA_ARGS__;}

// clang-format on

/**
 * @brief __[public]__ Detects if the compiler has exceptions enabled.
 *
 * Overridable by defining ``LF_COMPILER_EXCEPTIONS``.
 */
#ifndef LF_COMPILER_EXCEPTIONS
  #if defined(__cpp_exceptions) || (defined(_MSC_VER) && defined(_CPPUNWIND)) || defined(__EXCEPTIONS)
    #define LF_COMPILER_EXCEPTIONS 1
  #else
    #define LF_COMPILER_EXCEPTIONS 0
  #endif
#endif

#if LF_COMPILER_EXCEPTIONS || defined(LF_DOXYGEN_SHOULD_SKIP_THIS)
  /**
   * @brief Expands to ``try`` if exceptions are enabled, otherwise expands to ``if constexpr (true)``.
   */
  #define LF_TRY try
  /**
   * @brief Expands to ``catch (...)`` if exceptions are enabled, otherwise expands to ``if constexpr
   * (false)``.
   */
  #define LF_CATCH_ALL catch (...)
  /**
   * @brief Expands to ``throw X`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_THROW(X) throw X
  /**
   * @brief Expands to ``throw`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_RETHROW throw
#else
  #define LF_TRY if constexpr (true)
  #define LF_CATCH_ALL if constexpr (false)
  #ifndef NDEBUG
    #define LF_THROW(X) assert(false && "Tried to throw: " #X)
  #else
    #define LF_THROW(X) std::terminate()
  #endif
  #ifndef NDEBUG
    #define LF_RETHROW assert(false && "Tried to rethrow without compiler exceptions")
  #else
    #define LF_RETHROW std::terminate()
  #endif
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
 *    expression at runtime. Hence you should conservatively only use this macro
 *    if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */
#if __has_cpp_attribute(assume)
  #define LF_ASSUME(expr) [[assume(bool(expr))]]
#elif defined(__clang__)
  #define LF_ASSUME(expr) __builtin_assume(bool(expr))
#elif defined(__GNUC__) && !defined(__ICC)
  #define LF_ASSUME(expr)                                                                                                   \
    if (bool(expr)) {                                                                                                       \
    } else {                                                                                                                \
      __builtin_unreachable();                                                                                              \
    }
#elif defined(_MSC_VER) || defined(__ICC)
  #define LF_ASSUME(expr) __assume(bool(expr))
#else
  #define LF_ASSUME(expr)                                                                                                   \
    do {                                                                                                                    \
    } while (false)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  `` `` otherwise ``assert(expr)``.
 *
 * This is for expressions with side-effects.
 */
#ifndef NDEBUG
  #define LF_ASSERT_NO_ASSUME(expr) assert(expr)
#else
  #define LF_ASSERT_NO_ASSUME(expr)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  ``LF_ASSUME(expr)`` otherwise
 * ``assert(expr)``.
 */
#ifndef NDEBUG
  #define LF_ASSERT(expr) assert(expr)
#else
  #define LF_ASSERT(expr) LF_ASSUME(expr)
#endif

/**
 * @brief Macro to prevent a function to be inlined.
 */
#if !defined(LF_NOINLINE)
  #if defined(_MSC_VER)
    #define LF_NOINLINE __declspec(noinline)
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #if defined(__CUDACC__)
      // nvcc doesn't always parse __noinline__,
      // see: https://svn.boost.org/trac/boost/ticket/9392
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
 *    This does not imply the c++'s `inline` keyword which also has an effect on linkage.
 *
 *
 * \endrst
 */
#if !defined(LF_FORCEINLINE)
  #if defined(_MSC_VER)
    #define LF_FORCEINLINE __forceinline
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #define LF_FORCEINLINE __attribute__((__always_inline__))
  #else
    #define LF_FORCEINLINE
  #endif
#endif

/**
 * @brief This works-around https://github.com/llvm/llvm-project/issues/63022.
 */
#if defined(__clang__)
  #if defined(__apple_build_version__) || __clang_major__ <= 16
    #define LF_TLS_CLANG_INLINE LF_NOINLINE
  #else
    #define LF_TLS_CLANG_INLINE LF_FORCEINLINE
  #endif
#else
  #define LF_TLS_CLANG_INLINE LF_FORCEINLINE
#endif

/**
 * @brief __[public]__ A customizable logging macro.
 *
 * By default this is a no-op. Defining ``LF_DEFAULT_LOGGING`` will enable a default
 * logging implementation which prints to ``std::cout``. Overridable by defining your
 * own ``LF_LOG`` macro with an API like ``std::format()``.
 */
#ifndef LF_LOG
  #ifdef LF_DEFAULT_LOGGING
    #include <iostream>
    #include <thread>
    #include <type_traits>

    #ifdef __cpp_lib_format
      #include <format>
      #define LF_FORMAT(message, ...) std::format((message)__VA_OPT__(, ) __VA_ARGS__)
    #else
      #define LF_FORMAT(message, ...) (message)
    #endif

    #ifdef __cpp_lib_syncbuf
      #include <syncstream>
      #define LF_SYNC_COUT std::osyncstream(std::cout) << std::this_thread::get_id()
    #else
      #define LF_SYNC_COUT std::cout << std::this_thread::get_id()
    #endif

    #define LF_LOG(message, ...)                                                                                            \
      do {                                                                                                                  \
        if (!std::is_constant_evaluated()) {                                                                                \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n';                                    \
        }                                                                                                                   \
      } while (false)
  #else
    #define LF_LOG(head, ...)
  #endif
#endif

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */
