#pragma once

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
