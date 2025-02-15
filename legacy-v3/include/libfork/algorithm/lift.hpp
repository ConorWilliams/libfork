#ifndef B13463FB_3CF9_46F1_AFAC_19CBCB99A23C
#define B13463FB_3CF9_46F1_AFAC_19CBCB99A23C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for invocable
#include <functional>  // for invoke
#include <type_traits> // for invoke_result_t
#include <utility>     // for forward

#include "libfork/core/macro.hpp" // for LF_HOF_RETURNS, LF_STATIC_CALL
#include "libfork/core/task.hpp"  // for task

/**
 * @file lift.hpp
 *
 * @brief Higher-order functions for lifting functions into async functions.
 */

namespace lf {

/**
 * @brief A higher-order function that lifts a function into an asynchronous function.
 *
 * \rst
 *
 * This is useful for when you want to fork a regular function:
 *
 * .. code::
 *
 *    auto work(int x) -> int;
 *
 * Then later in some async context you can do:
 *
 * .. code::
 *
 *    {
 *      int a, b;
 *
 *      co_await fork[a, lift](work, 42);
 *      co_await fork[b, lift](work, 007);
 *
 *      co_await join;
 *    }
 *
 * .. note::
 *
 *    The lifted function will accept arguments by forwarding reference.
 *
 * \endrst
 */
inline constexpr auto lift = []<class F, class... Args>(auto, F &&func, Args &&...args)
                                 LF_STATIC_CALL -> task<std::invoke_result_t<F, Args...>>
  requires std::invocable<F, Args...>
{
  co_return std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
};

/**
 * @brief Lift an overload-set/template into a constrained lambda.
 *
 * This is useful for passing overloaded/template names to higher order functions like `lf::fork`/`lf::call`.
 */
#define LF_LOFT(name)                                                                                        \
  [](auto &&...args) LF_STATIC_CALL LF_HOF_RETURNS(name(::std::forward<decltype(args)>(args)...))

/**
 * @brief Lift an overload-set/template into a constrained capturing lambda.
 *
 * The variadic arguments are used as the lambda's capture.
 *
 * This is useful for passing overloaded/template names to higher order functions like `lf::fork`/`lf::call`.
 */
#define LF_CLOFT(name, ...)                                                                                  \
  [__VA_ARGS__](auto &&...args) LF_HOF_RETURNS(name(::std::forward<decltype(args)>(args)...))

} // namespace lf

#endif /* B13463FB_3CF9_46F1_AFAC_19CBCB99A23C */
