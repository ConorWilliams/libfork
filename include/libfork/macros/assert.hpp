
#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility> // for unreachable

#ifndef LF_USE_LIBASSERT
  #include <cassert>
#else
  #include <libassert/assert.hpp>
#endif

/**
 * @file assert.hpp
 *
 * @brief Internal assertion macros
 *
 * These are exhaustively documented due to macros nasty visibility rules
 * however, only macros that are marked as __[public]__ should be consumed.
 */

/*
 * @brief Equiviland to assert(...)
 */
#ifndef LF_USE_LIBASSERT
  #define LF_DEBUG_ASSERT(expr, ...) assert(expr)
#else
  #define LF_DEBUG_ASSERT(...) DEBUG_ASSERT(__VA_ARGS__)
#endif

/**
 * @brief Invokes undefined behavior if ``expr`` evaluates to `false`.
 *
 * \rst
 *
 *  .. warning::
 *
 *    This has different semantics than ``[[assume(expr)]]`` as it WILL evaluate
 *    the expression at runtime. Hence you should conservatively only use this
 *    macro if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */

#define LF_ASSUME(expr)                                                                            \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      std::unreachable();                                                                          \
    }                                                                                              \
  } while (false)

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  `` `` otherwise
 * ``assert(expr)``.
 *
 * This is for expressions with side-effects.
 */
#ifndef NDEBUG
  #define LF_JUST_ASSERT(...) LF_DEBUG_ASSERT(__VA_ARGS__)
#else
  #define LF_JUST_ASSERT(...)                                                                      \
    do {                                                                                           \
    } while (false)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is
 * ``LF_ASSUME(expr)`` otherwise ``assert(expr)``.
 */
#ifndef NDEBUG
  #define LF_ASSERT(...) LF_DEBUG_ASSERT(__VA_ARGS__)
#else
  #define LF_ASSERT(expr, ...) LF_ASSUME(expr)
#endif

/**
 * @brief Assume that A implies B.
 */
#define LF_ASSERT_IMPLIES(a, b, ...) LF_ASSERT((!(a) || (b)) __VA_OPT__(, ) __VA_ARGS__)

// NOLINTEND
