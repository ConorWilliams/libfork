#ifndef D336C448_D1EE_4616_9277_E0D7D550A10A
#define D336C448_D1EE_4616_9277_E0D7D550A10A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

#include "libfork/core.hpp"

/**
 * @file concepts.hpp
 *
 * @brief Variations of the standard library's concepts used for constraining algorithms.
 */

namespace lf {

// ------------------------------------ invoke_result_t ------------------------------------ //

namespace impl::detail {

template <typename F, typename... Args>
struct task_result : std::invoke_result<F, Args...> {};

template <async_fn F, typename... Args>
struct task_result<F, Args...> {
  using type = value_of<std::invoke_result_t<F, Args...>>;
};

} // namespace impl::detail

/**
 * @brief The result of invoking a regular-or-async function.
 *
 * If F is a regular function then this is the same as `std::invoke_result<F, Args...>`. Otherwise,
 * if F is an async function then this is the same as the result of `co_await`ing a value of type
 * `std::invoke_result_t<F, Args...>` in an `lf::task`.
 */
template <typename F, typename... Args>
  requires std::invocable<F, Args...>
using invoke_result_t = typename impl::detail::task_result<F, Args...>::type;

// ------------------------------------ indirectly_result_t ------------------------------------ //

/**
 * @brief A`std::indirect_result_t` that uses `lf::invoke_result_t` instead of the `std` version.
 */
template <class F, class... Is>
  requires (std::indirectly_readable<Is> && ...) && std::invocable<F, std::iter_reference_t<Is>...>
using indirect_result_t = invoke_result_t<F, std::iter_reference_t<Is>...>;

// ------------------------------- indirectly_unary_invocable ------------------------------- //

/**
 * @brief A `std::indirectly_unary_invocable` that uses `lf::invoke_result_t` instead of the `std` version.
 */
template <class F, class I>
concept indirectly_unary_invocable = std::indirectly_readable<I> &&                          //
                                     std::copy_constructible<F> &&                           //
                                     std::invocable<F &, std::iter_value_t<I> &> &&          //
                                     std::invocable<F &, std::iter_reference_t<I>> &&        //
                                     std::invocable<F &, std::iter_common_reference_t<I>> && //
                                     std::common_reference_with<                             //
                                         invoke_result_t<F &, std::iter_value_t<I> &>,       //
                                         invoke_result_t<F &, std::iter_reference_t<I>>      //
                                         >;

// ------------------------------------------- Magma ------------------------------------------- //

namespace impl::detail {

template <class Bop, class R, class T>
concept magma_help = std::convertible_to<T, R> &&                   //
                     std::regular_invocable<Bop, T, R> &&           //
                     std::regular_invocable<Bop, R, T> &&           //
                     std::regular_invocable<Bop, R, R> &&           //
                     std::same_as<R, invoke_result_t<Bop, T, R>> && //
                     std::same_as<R, invoke_result_t<Bop, R, T>> && //
                     std::same_as<R, invoke_result_t<Bop, R, R>>;   //

} // namespace impl::detail

/**
 * @brief A magma is a set `S` and an associative binary operation `·`, such that `S` is closed under `·`.
 *
 * Example: `(Z, +)` is a magma, since we can add any two integers and the result is also an integer.
 *
 * Example: `(Z, /)` is not a magma, since `2/3` s not an integer.
 *
 * Let `bop` and `t` be objects of types `Bop` and `T` respectively. Then the following expressions
 * must be valid:
 *
 * 1. `bop(t, t)`
 * 2. `bop(t, bop(t, t))`
 * 3. `bop(bop(t, t), t)`
 * 4. `bop(bop(t, t), bop(t, t))`
 *
 * Additionally, the expressions must return the same type, `R`, which `t` must be convertible to.
 *
 * Hence the `S` is the set of the values in `R` and the values in `T` _represent_ a subset of `S`.
 */
template <class Bop, class T>
concept magma =
    std::regular_invocable<Bop, T, T> && impl::detail::magma_help<Bop, invoke_result_t<Bop, T, T>, T>;

namespace detail::static_test {

static_assert(magma<std::plus<>, int>);
static_assert(magma<std::plus<>, float>);
static_assert(magma<std::plus<double>, float>);

struct bad_plus {
  auto operator()(int, int) const -> char;
  auto operator()(char, char) const -> int;
};

static_assert(!magma<bad_plus, int>);

} // namespace detail::static_test

/**
 * @brief An indirect magma is like a magma but applying the operator to the types that `I` reference.
 */
template <class Bop, class I>
concept indirect_magma = std::indirectly_readable<I> &&                       //
                         std::copy_constructible<Bop> &&                      //
                         magma<Bop &, std::iter_value_t<I> &> &&              //
                         magma<Bop &, std::iter_reference_t<I>> &&            //
                         magma<Bop &, std::iter_common_reference_t<I>> &&     //
                         std::common_reference_with<                          //
                             invoke_result_t<Bop &, std::iter_value_t<I> &>,  //
                             invoke_result_t<Bop &, std::iter_reference_t<I>> //
                             >;                                               //

// ------------------------------------ Semigroup ------------------------------------ //

/**
 * @brief A semigroup is an `lf::magma` where the binary operation is associative.
 *
 * Associativity means that for all `a, b, c` in `S`, `(a · b) · c = a · (b · c)`.
 *
 * Example: `(Z, -)` is magma but not a semigroup, since `(1 - 1) - 1 != 1 - (1 - 1)`.
 *
 * This is a semantic only requirement.
 */
template <class Bop, class T>
concept semigroup = magma<Bop, T>; // subsume magma

/**
 * @brief An indirect semigroup is like a semigroup but applying the operator to the types that `I` reference.
 */
template <class Bop, class I>
concept indirect_semigroup = indirect_magma<Bop, I> &&                          // subsume indirect_magma
                             semigroup<Bop &, std::iter_value_t<I> &> &&        //
                             semigroup<Bop &, std::iter_reference_t<I>> &&      //
                             semigroup<Bop &, std::iter_common_reference_t<I>>; //

} // namespace lf

#endif /* D336C448_D1EE_4616_9277_E0D7D550A10A */
