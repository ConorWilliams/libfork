#pragma once

#include "libfork/__impl/exception.hpp"

/**
 * @file assume.hpp
 *
 * @brief A collection of internal macros.
 *
 * These macros are not safe to use unless `import std` is in scope.
 */

/**
 * @brief If expr evaluates to `false`, terminates the program with an error message.
 *
 * This macro is always active, regardless of optimization settings or `NDEBUG`.
 */
#define LF_ENSURE(...)                                                                                       \
  do {                                                                                                       \
    if (!(__VA_ARGS__)) {                                                                                    \
      LF_TERMINATE("Assumption '" #__VA_ARGS__ "' failed!");                                                 \
    }                                                                                                        \
  } while (false)

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
#ifdef NDEBUG
  #define LF_ASSUME(...)                                                                                     \
    do {                                                                                                     \
      if (!(__VA_ARGS__)) {                                                                                  \
        ::std::unreachable();                                                                                \
      }                                                                                                      \
    } while (false)
#else
  #define LF_ASSUME(...) LF_ENSURE(__VA_ARGS__)
#endif
