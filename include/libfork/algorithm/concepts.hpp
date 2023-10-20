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
 * @brief A version of `std::indirect_result_t` that uses `lf::invoke_result_t` instead of the `std` version.
 */
template <class F, class... Is>
  requires (std::indirectly_readable<Is> && ...) && std::invocable<F, std::iter_reference_t<Is>...>
using indirect_result_t = invoke_result_t<F, std::iter_reference_t<Is>...>;

// ------------------------------- indirectly_unary_invocable ------------------------------- //

/**
 * @brief A version of `std::indirectly_unary_invocable` that uses `lf::invoke_result_t` instead of the `std`
 * version.
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

// ---------------------------------- Semigroup  helpers ---------------------------------- //

namespace impl {

namespace detail {

template <typename...>
inline constexpr bool all_same_impl = false;

template <>
inline constexpr bool all_same_impl<> = true;

template <typename T>
inline constexpr bool all_same_impl<T> = true;

template <typename T, typename... Ts>
inline constexpr bool all_same_impl<T, T, Ts...> = all_same_impl<T, Ts...>;

} // namespace detail

template <typename... Ts>
concept all_same_as = detail::all_same_impl<Ts...>;

/**
 * @brief Test if `Bop` is invocable with all combinations of `T` and `U` and invocations return `R`.
 *
 * This is commutative in `T` and `U`.
 */
template <class R, class Bop, class T, class U>
concept mixed_invocable_to = std::regular_invocable<Bop, U, U> && //
                             std::regular_invocable<Bop, T, U> && //
                             std::regular_invocable<Bop, U, T> && //
                             std::regular_invocable<Bop, U, U> && //
                             all_same_as<                         //
                                 R,                               //
                                 invoke_result_t<Bop, T, T>,      //
                                 invoke_result_t<Bop, T, U>,      //
                                 invoke_result_t<Bop, U, T>,      //
                                 invoke_result_t<Bop, U, U>       //
                                 >;                               //

template <class R, class Bop, class T>
concept semigroup_help = mixed_invocable_to<R, Bop, T, R>;

} // namespace impl

// ---------------------------------- Semigroup ---------------------------------- //

/**
 * @brief A semigroup is a set `S` and an associative binary operation `·`, such that `S` is closed under `·`.
 *
 * Associativity means that for all `a, b, c` in `S`, `(a · b) · c = a · (b · c)`.
 *
 * Example: `(Z, +)` is a semigroup, since we can add any two integers and the result is also an integer.
 *
 * Example: `(Z, /)` is not a semigroup, since `2/3` s not an integer.
 *
 * Example: `(Z, -)` is not a semigroup, since `(1 - 1) - 1 != 1 - (1 - 1)`.
 *
 * Let `bop` and `t` be objects of types `Bop` and `T` respectively. Then the following expressions
 * must be valid:
 *
 * 1. `bop(t, t)`
 * 2. `bop(t, bop(t, t))`
 * 3. `bop(bop(t, t), t)`
 * 4. `bop(bop(t, t), bop(t, t))`
 *
 * Additionally, the expressions must return the same type, `R`.
 *
 * Hence the `S` is the set of the values in `R`, to align with the mathematical definition of a semigroup
 * we say that `T` _represents_ `S`.
 *
 * __Note:__ A semigroup requires all invocations to be regular. This is a semantic requirement only.
 */
template <class Bop, class T>
concept semigroup = std::regular_invocable<Bop, T, T> &&                      //
                    impl::semigroup_help<invoke_result_t<Bop, T, T>, Bop, T>; //

namespace impl {

/**
 * @brief The result of invoking a semigroup's binary operator with two values of type `T`.
 */
template <class Bop, class T>
  requires semigroup<Bop, T>
using semigroup_result_t = invoke_result_t<Bop, T, T>;

/**
 * @brief Test if a binary operator is a dual semigroup.
 *
 * A dual semigroup requires that `Bop` is a semigroup over `T` and `U` with the same
 * `semigroup_result_t` and mixed invocation of `Bop` over `T` and `U` have semigroup semantics.
 *
 * This is commutative in `T` and `U`.
 */
template <class Bop, class T, class U>
concept dual_semigroup = semigroup<Bop, T> &&                                         //
                         semigroup<Bop, U> &&                                         //
                         mixed_invocable_to<semigroup_result_t<Bop, U>, Bop, T, U> && //
                         mixed_invocable_to<semigroup_result_t<Bop, T>, Bop, U, T>;   //

} // namespace impl

/**
 * @brief An indirect semigroup is like a `lf::semigroup` but applying the operator to the types that `I`
 * references.
 */
template <class Bop, class I>
concept indirect_semigroup =
    std::indirectly_readable<I> &&                                                            //
    std::copy_constructible<Bop> &&                                                           //
    semigroup<Bop &, std::iter_value_t<I> &> &&                                               //
    semigroup<Bop &, std::iter_reference_t<I>> &&                                             //
    semigroup<Bop &, std::iter_common_reference_t<I>> &&                                      //
    impl::dual_semigroup<Bop &, std::iter_value_t<I> &, std::iter_reference_t<I>> &&          //
    impl::dual_semigroup<Bop &, std::iter_reference_t<I>, std::iter_common_reference_t<I>> && //
    impl::dual_semigroup<Bop &, std::iter_common_reference_t<I>, std::iter_value_t<I> &>;     //

// -------------- Quick tests -------------- //

namespace impl::detail::static_test {

static_assert(semigroup<std::plus<>, int>);
static_assert(semigroup<std::plus<>, float>);
static_assert(semigroup<std::plus<double>, float>);

struct bad_plus {
  auto operator()(int, int) const -> char;
  auto operator()(char, char) const -> int;
};

static_assert(!semigroup<bad_plus, int>);

} // namespace impl::detail::static_test

// ------------------------------------ Foldable ------------------------------------ //

/**
 * @brief Test if a binary operation supports a fold operation over a type.
 *
 * This means a collection of one or more values of type `T` can be folded to a single value
 * of type `R` equal to `lf::invoke_result_t<Bop, T, T>` using `bop`, an operator of type `Bop`.
 *
 * For example using the infix notation `a · b` to mean `bop(a, b)`:
 *
 * 1. `fold bop [a] = a`
 * 2. `fold bop [a, b] = a · b`
 * 3. `fold bop [a, b, c] = a · b · c`
 * 4. `fold bop [a, b, c, ...] = a · b · c · ...`
 *
 * The order of evaluation is unspecified but the elements will not be reordered.
 *
 * This requires that `Bop` and `T` form a semigroup, `R` is a `std::movable` type and `T`
 * is convertible to `R`, as required for the case of only a single value.
 */
template <class Bop, class T>
concept foldable = semigroup<Bop, T> &&                                //
                   std::movable<invoke_result_t<Bop, T, T>> &&         //
                   std::convertible_to<T, invoke_result_t<Bop, T, T>>; //

/**
 * @brief An indirect foldable is like a `lf::foldable` but applying the operator to the types that `I`
 * references.
 */
template <class Bop, class I>
concept indirectly_foldable = indirect_semigroup<Bop, I> &&                     //
                              foldable<Bop &, std::iter_value_t<I> &> &&        //
                              foldable<Bop &, std::iter_reference_t<I>> &&      //
                              foldable<Bop &, std::iter_common_reference_t<I>>; //

} // namespace lf

#endif /* D336C448_D1EE_4616_9277_E0D7D550A10A */
