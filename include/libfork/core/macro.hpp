#ifndef C5DCA647_8269_46C2_B76F_5FA68738AEDA
#define C5DCA647_8269_46C2_B76F_5FA68738AEDA

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cassert> // for assert
#include <version> // for __cpp_lib_unreachable, ...

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
 * Increments with incompatible API changes.
 */
#define LF_VERSION_MAJOR 3
/**
 * @brief __[public]__ The minor version of libfork.
 *
 * Increments when functionality is added in an API backward compatible manner.
 */
#define LF_VERSION_MINOR 8
/**
 * @brief __[public]__ The patch version of libfork.
 *
 * Increments when bug fixes are made in an API backward compatible manner.
 */
#define LF_VERSION_PATCH 2

/**
 * @brief Use to conditionally decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) with
 * ``static``.
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

  #include <exception>

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

#ifdef __cpp_lib_unreachable
  #include <utility>
#endif

namespace lf::impl {

#ifdef __cpp_lib_unreachable
using std::unreachable;
#else
/**
 * @brief A homebrew version of `std::unreachable`, see https://en.cppreference.com/w/cpp/utility/unreachable
 */
[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  #if defined(_MSC_VER) && !defined(__clang__) // MSVC
  __assume(false);
  #else                                        // GCC, Clang
  __builtin_unreachable();
  #endif
  // Even if no extension is used, undefined behavior is still raised by infinite loop.
  for (;;) {
  }
}
#endif

} // namespace lf::impl

/**
 * @brief Invokes undefined behavior if ``expr`` evaluates to `false`.
 *
 * \rst
 *
 *  .. warning::
 *
 *    This has different semantics than ``[[assume(expr)]]`` as it WILL evaluate the
 *    expression at runtime. Hence you should conservatively only use this macro
 *    if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */

#define LF_ASSUME(expr)                                                                                      \
  do {                                                                                                       \
    if (!(expr)) {                                                                                           \
      ::lf::impl::unreachable();                                                                             \
    }                                                                                                        \
  } while (false)

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  `` `` otherwise ``assert(expr)``.
 *
 * This is for expressions with side-effects.
 */
#ifndef NDEBUG
  #define LF_ASSERT_NO_ASSUME(expr) assert(expr)
#else
  #define LF_ASSERT_NO_ASSUME(expr)                                                                          \
    do {                                                                                                     \
    } while (false)
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
  #if defined(_MSC_VER) && !defined(__clang__)
    #define LF_NOINLINE __declspec(noinline)
  #elif defined(__GNUC__) && __GNUC__ > 3
  // Clang also defines __GNUC__ (as 4)
    #if defined(__CUDACC__)
  // nvcc doesn't always parse __noinline__, see: https://svn.boost.org/trac/boost/ticket/9392
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
 * @brief Force no-inline for clang, works-around https://github.com/llvm/llvm-project/issues/63022.
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

/**
 * @brief Macro to use next to 'inline' to force a function to be inlined.
 *
 * \rst
 *
 * .. note::
 *
 *    This does not imply the c++'s `inline` keyword which also has an effect on linkage.
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

#if defined(__clang__) && defined(__has_attribute)
  /**
   * @brief Compiler specific attribute.
   */
  #if __has_attribute(coro_return_type)
    #define LF_CORO_RETURN_TYPE [[clang::coro_return_type]]
  #else
    #define LF_CORO_RETURN_TYPE
  #endif
  /**
   * @brief Compiler specific attribute.
   */
  #if __has_attribute(coro_only_destroy_when_complete)
    #define LF_CORO_ONLY_DESTROY_WHEN_COMPLETE [[clang::coro_only_destroy_when_complete]]
  #else
    #define LF_CORO_ONLY_DESTROY_WHEN_COMPLETE
  #endif

  /**
   * @brief Compiler specific attributes libfork uses for its coroutine types.
   */
  #define LF_CORO_ATTRIBUTES LF_CORO_RETURN_TYPE LF_CORO_ONLY_DESTROY_WHEN_COMPLETE

#else
  /**
   * @brief Compiler specific attributes libfork uses for its coroutine types.
   */
  #define LF_CORO_ATTRIBUTES
#endif

/**
 * @brief Compiler specific attributes libfork uses for its coroutine types.
 */
#if defined(__clang__) && defined(__has_attribute)
  #if __has_attribute(coro_wrapper)
    #define LF_CORO_WRAPPER [[clang::coro_wrapper]]
  #else
    #define LF_CORO_WRAPPER
  #endif
#else
  #define LF_CORO_WRAPPER
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

    #define LF_LOG(message, ...)                                                                             \
      do {                                                                                                   \
        if (!std::is_constant_evaluated()) {                                                                 \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n';                     \
        }                                                                                                    \
      } while (false)
  #else
    #define LF_LOG(head, ...)
  #endif
#endif

/**
 * @brief Concatenation macro
 */
#define LF_CONCAT_OUTER(a, b) LF_CONCAT_INNER(a, b)
/**
 * @brief Internal concatenation macro (use LF_CONCAT_OUTER)
 */
#define LF_CONCAT_INNER(a, b) a##b

/**
 * @brief Depreciate operator() in favor of operator[] if multidimensional subscript is available.
 */
#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  #define LF_DEPRECATE_CALL [[deprecated("Use operator[] instead of operator()")]]
#else
  #define LF_DEPRECATE_CALL
#endif

/**
 * @brief Expands to ``_Pragma(#x)``.
 */
#define LF_AS_PRAGMA(x) _Pragma(#x)

/**
 * @brief Expands to `#pragma unroll n` or equivalent if the compiler supports it.
 */
#ifdef __clang__
  #define LF_PRAGMA_UNROLL(n) LF_AS_PRAGMA(unroll n)
#elif defined(__GNUC__)
  #define LF_PRAGMA_UNROLL(n) LF_AS_PRAGMA(GCC unroll n)
#else
  #define LF_PRAGMA_UNROLL(n)
#endif

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */
