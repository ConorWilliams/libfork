#pragma once

/**
 * @file macro.hpp
 *
 * @brief A collection of internal/public macros.
 *
 * These are exhaustively documented due to macros nasty visibility rules
 * however, only macros that are marked as __[public]__ should be consumed.
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

// =============== Exception Handling Macros =============== //

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

#if LF_COMPILER_EXCEPTIONS
  /**
   * @brief Expands to ``try`` if exceptions are enabled, otherwise expands to ``if constexpr (true)``.
   */
  #define LF_TRY try
  /**
   * @brief Expands to ``catch (...)`` if exceptions are enabled, otherwise ``if constexpr (false)``.
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

  #include <cassert>
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
