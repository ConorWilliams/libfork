#pragma once

/**
 * @file exception.hpp
 *
 * @brief A collection of internal macros for exception handling.
 *
 * These macros are standalone i.e. they can be used without importing/including anything else.
 */

/**
 * @brief Detects if the compiler has exceptions enabled.
 *
 * Overridable by defining `LF_COMPILER_EXCEPTIONS` globally.
 */
#ifndef LF_COMPILER_EXCEPTIONS
  #if defined(__cpp_exceptions) || (defined(_MSC_VER) && defined(_CPPUNWIND)) || defined(__EXCEPTIONS)
    #define LF_COMPILER_EXCEPTIONS 1
  #else
    #define LF_COMPILER_EXCEPTIONS 0
  #endif
#endif

namespace lf::impl {

/**
 * @brief Calls `std::terminate` after printing `msg`.
 */
[[noreturn]] void terminate_with(char const *message, char const *file, int line) noexcept;

} // namespace lf::impl

#define LF_TERMINATE(message) ::lf::impl::terminate_with((message), __FILE__, __LINE__)

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
  /**
   * @brief Expands to ``try`` if exceptions are enabled, otherwise expands to ``if constexpr (true)``.
   */
  #define LF_TRY if constexpr (true)
  /**
   * @brief Expands to ``catch (...)`` if exceptions are enabled, otherwise ``if constexpr (false)``.
   */
  #define LF_CATCH_ALL if constexpr (false)
  /**
   * @brief Expands to ``throw X`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_THROW(X) LF_TERMINATE("Tried to throw '" #X "' without compiler exceptions")
  /**
   * @brief Expands to ``throw`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_RETHROW LF_TERLF_TERMINATE("Tried to rethrow without compiler exceptions")
#endif
