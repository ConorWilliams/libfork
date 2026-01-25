#pragma once

#include "libfork/__impl/exception.hpp"

/**
 * @file compiler.hpp
 *
 * @brief A collection of internal macros.
 *
 * These macros are not safe to use unless `import std` is in scope.
 */

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
  #define LF_ASSUME(expr)                                                                                    \
    if (!(expr)) {                                                                                           \
      ::std::unreachable();                                                                                  \
    }
#else
  #define LF_ASSUME(expr)                                                                                    \
    do {                                                                                                     \
      if (!(expr)) {                                                                                         \
        LF_TERMINATE("Assumption '" #expr "' failed!");                                                      \
      }                                                                                                      \
    } while (false)
#endif
