#ifndef B13463FB_3CF9_46F1_AFAC_19CBCB99A23C
#define B13463FB_3CF9_46F1_AFAC_19CBCB99A23C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <source_location>
#include <type_traits>

#include "libfork/core.hpp"

namespace lf {

namespace impl {

template <typename F>
struct lifted {
 private:
  template <typename... Args>
  using task_t = task<std::invoke_result_t<F, Args...>, "lf::lift">;

 public:
  template <first_arg Head, typename... Args>
    requires (tag_of<Head> != tag::fork && std::invocable<F, Args...>)
  LF_STATIC_CALL auto operator()(Head, Args &&...args) LF_STATIC_CONST->task_t<Args...> {
    co_return std::invoke(F{}, std::forward<Args>(args)...);
  }

  template <typename Head, typename... Args>
    requires (tag_of<Head> == tag::fork && std::invocable<F, Args...>)
  LF_STATIC_CALL auto operator()(Head, Args... args) LF_STATIC_CONST->task_t<Args...> {
    co_return std::invoke(F{}, std::move(args)...);
  }
};

} // namespace impl

/**
 * @brief A higher-order function that lifts a function into an ``async`` function.
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
 *      co_await fork[a, lift(work)](42);
 *      co_await fork[b, lift(work)](007);
 *
 *      co_await join;
 *    }
 *
 * .. note::
 *
 *    The lifted function will accept arguments by-value if it is forked and by forwarding reference otherwise.
 *    This is to prevent dangling, use ``std::ref`` if you actually want a reference.
 *
 * \endrst
 */
template <stateless F>
consteval auto lift(F) -> async<impl::lifted<F>> {
  return {};
}

/**
 * @brief Lift an overload-set/template into a constrained lambda.
 *
 * This is useful for passing overloaded/template functions to higher order functions like `lf::fork`, `lf::call` etc.
 */
#define LF_LIFT(overload_set)                                                                                               \
  [](auto &&...args) LF_STATIC_CALL LF_HOF_RETURNS(overload_set(std::forward<decltype(args)>(args)...))

/**
 * @brief Lift an overload-set/template into an async function, equivalent to `lf::lift(LF_LIFT(overload_set))`.
 */
#define LF_LIFT2(overload_set) ::lf::lift(LF_LIFT(overload_set))

} // namespace lf

#endif /* B13463FB_3CF9_46F1_AFAC_19CBCB99A23C */
