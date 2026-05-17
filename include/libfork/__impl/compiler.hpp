#pragma once

#include "libfork/__impl/exception.hpp"

/**
 * @file compiler.hpp
 *
 * @brief A collection of internal macros.
 *
 * These macros are standalone i.e. they can be used without importing/including anything else.
 */

// =============== Inlining/optimization =============== //

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
#if !defined(LF_FORCE_INLINE)
  #if defined(_MSC_VER) && !defined(__clang__)
    #define LF_FORCE_INLINE __forceinline
  #elif defined(__GNUC__) && __GNUC__ > 3
  // Clang also defines __GNUC__ (as 4)
    #define LF_FORCE_INLINE __attribute__((__always_inline__))
  #else
    #define LF_FORCE_INLINE
  #endif
#endif

/**
 * @brief Macro to prevent a function to be inlined.
 */
#if !defined(LF_NO_INLINE)
  #if defined(_MSC_VER) && !defined(__clang__)
    #define LF_NO_INLINE __declspec(noinline)
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #if defined(__CUDACC__)
      // nvcc doesn't always parse __noinline__, see: https://svn.boost.org/trac/boost/ticket/9392
      #define LF_NO_INLINE __attribute__((noinline))
    #elif defined(__HIP__)
      // See https://github.com/boostorg/config/issues/392
      #define LF_NO_INLINE __attribute__((noinline))
    #else
      #define LF_NO_INLINE __attribute__((__noinline__))
    #endif
  #else
    #define LF_NO_INLINE
  #endif
#endif
