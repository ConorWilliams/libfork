#pragma once

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>

/**
 * @file exceptions.hpp
 *
 * @brief Macros for handling exceptions
 *
 * These are exhaustively documented due to macros nasty visibility rules
 * however, only macros that are marked as __[public]__ should be consumed.
 */

/**
 * @brief __[public]__ Detects if the compiler has exceptions enabled.
 *
 * Overridable by defining ``LF_COMPILER_EXCEPTIONS``.
 */
#ifndef LF_COMPILER_EXCEPTIONS
  #if defined(__cpp_exceptions) || (defined(_MSC_VER) && defined(_CPPUNWIND)) ||                   \
      defined(__EXCEPTIONS)
    #define LF_COMPILER_EXCEPTIONS 1
  #else
    #define LF_COMPILER_EXCEPTIONS 0
  #endif
#endif

#if LF_COMPILER_EXCEPTIONS || defined(LF_DOXYGEN_SHOULD_SKIP_THIS)
  /**
   * @brief Expands to ``try`` if exceptions are enabled, otherwise expands to
   * ``if constexpr (true)``.
   */
  #define LF_TRY try
  /**
   * @brief Expands to ``catch (...)`` if exceptions are enabled, otherwise
   * expands to ``if constexpr (false)``.
   */
  #define LF_CATCH_ALL catch (...)
  /**
   * @brief Expands to ``throw X`` if exceptions are enabled, otherwise
   * terminates the program.
   */
  #define LF_THROW(X) throw X
  /**
   * @brief Expands to ``throw`` if exceptions are enabled, otherwise terminates
   * the program.
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
