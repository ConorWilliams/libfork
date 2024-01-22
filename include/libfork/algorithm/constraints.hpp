#ifndef D336C448_D1EE_4616_9277_E0D7D550A10A
#define D336C448_D1EE_4616_9277_E0D7D550A10A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for copy_constructible, common_reference_with, invocable
#include <iterator>    // for iter_reference_t, indirectly_readable, iter_differ...
#include <type_traits> // for decay_t, invoke_result, remove_cvref_t

#include "libfork/core/invocable.hpp" // for async_invocable, async_regular_invocable, async_re...

/**
 * @file constraints.hpp
 *
 * @brief Variations of the standard library's concepts used for constraining algorithms.
 */

namespace lf {

// ------------------------------------  either invocable ------------------------------------ //

/**
 * @brief Test if "F" is async invocable __xor__ normally invocable with ``Args...``.
 */
template <typename F, typename... Args>
concept invocable = (std::invocable<F, Args...> || async_invocable<F, Args...>)&&!(
    std::invocable<F, Args...> && async_invocable<F, Args...>);

/**
 * @brief Test if "F" is regularly async invocable __xor__ normally invocable invocable with ``Args...``.
 */
template <typename F, typename... Args>
concept regular_invocable = (std::regular_invocable<F, Args...> || async_regular_invocable<F, Args...>)&&!(
    std::regular_invocable<F, Args...> && async_regular_invocable<F, Args...>);

// ------------------------------------  either result type ------------------------------------ //

namespace detail {

template <typename F, typename... Args>
struct either_invocable_result;

template <typename F, typename... Args>
  requires async_invocable<F, Args...>
struct either_invocable_result<F, Args...> : async_result<F, Args...> {};

template <typename F, typename... Args>
  requires std::invocable<F, Args...>
struct either_invocable_result<F, Args...> : std::invoke_result<F, Args...> {};

} // namespace detail

/**
 * @brief The result of invoking a regular-or-async function.
 *
 * If F is a regular function then this is the same as `std::invoke_result<F, Args...>`. Otherwise,
 * if F is an async function then this is the same as `lf::core::invoke_result_t<F, Args...>`.
 */
template <typename F, typename... Args>
  requires invocable<F, Args...>
using invoke_result_t = typename detail::either_invocable_result<F, Args...>::type;

// ------------------------------------ indirect_value_t ------------------------------------ //

namespace detail {

/**
 * @brief Base case for regular iterators.
 */
template <typename I>
struct indirect_value_impl {
  using type = std::iter_value_t<I> &;
};

/**
 * @brief Specialization for projected iterators.
 */
template <typename Proj>
  requires requires { typename Proj::secret_projected_indirect_value_helper; }
struct indirect_value_impl<Proj> {
 private:
  using iter = typename Proj::secret_projected_indirect_value_helper::iterator;
  using proj = typename Proj::secret_projected_indirect_value_helper::projection;

 public:
  // Recursively drill down to the non-projected iterator.
  using type = invoke_result_t<proj &, typename indirect_value_impl<iter>::type>;
};

} // namespace detail

/**
 * @brief From [P2609R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2609r3.html), the
 * referenced value type.
 *
 * Relaxes some constraints for ``lf::core::indirectly_unary_invocable`` Specifically: `indirect_value_t<I>`
 * must be `std::iter_value_t<I> &` for an iterator and `invoke_result_t<Proj &, indirect_value_t<Iter>>` for
 * `projected<Proj, Iter>`.
 */
template <std::indirectly_readable I>
using indirect_value_t = typename detail::indirect_value_impl<I>::type;

// ------------------------------- indirectly_unary_invocable ------------------------------- //

/**
 * @brief ``std::indirectly_unary_invocable` that accepts async and regular function.
 *
 * This uses the relaxed version from
 * [P2997R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2997r0.html#ref-LWG3859) and the
 * further relaxation from [P2609R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2609r3.html)
 */
template <class F, class I>
concept indirectly_unary_invocable = std::indirectly_readable<I> &&                     //
                                     std::copy_constructible<F> &&                      //
                                     invocable<F &, indirect_value_t<I>> &&             //
                                     invocable<F &, std::iter_reference_t<I>> &&        //
                                     std::common_reference_with<                        //
                                         invoke_result_t<F &, indirect_value_t<I>>,     //
                                         invoke_result_t<F &, std::iter_reference_t<I>> //
                                         >;

/**
 * @brief ``std::indirectly_regular_unary_invocable` that accepts async and regular function.
 *
 * This uses the relaxed version from
 * [P2997R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2997r0.html#ref-LWG3859) and the
 * further relaxation from [P2609R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2609r3.html)
 *
 * __Hint:__ `indirect_value_t<I> = invoke_result_t<proj &, std::iter_value_t<I> &>` for 1-projected
 * iterators.
 */
template <class F, class I>
concept indirectly_regular_unary_invocable = std::indirectly_readable<I> &&                      //
                                             std::copy_constructible<F> &&                       //
                                             regular_invocable<F &, indirect_value_t<I>> &&      //
                                             regular_invocable<F &, std::iter_reference_t<I>> && //
                                             std::common_reference_with<                         //
                                                 invoke_result_t<F &, indirect_value_t<I>>,      //
                                                 invoke_result_t<F &, std::iter_reference_t<I>>  //
                                                 >;

// ------------------------------------ indirect_result_t ------------------------------------ //

/**
 * @brief A variation of `std::indirect_result_t` that accepts async and regular function.
 */
template <class F, class... Is>
  requires (std::indirectly_readable<Is> && ...) && invocable<F, std::iter_reference_t<Is>...>
using indirect_result_t = invoke_result_t<F, std::iter_reference_t<Is>...>;

// ------------------------------------ projected ------------------------------------ //

namespace detail {

template <class I>
struct conditional_difference_type {};

template <std::weakly_incrementable I>
struct conditional_difference_type<I> {
  using difference_type = std::iter_difference_t<I>;
};

template <class I, class Proj>
struct projected_impl {
  struct adl_barrier : conditional_difference_type<I> {

    using value_type = std::remove_cvref_t<indirect_result_t<Proj &, I>>;

    auto operator*() const -> indirect_result_t<Proj &, I>; // not defined

    struct secret_projected_indirect_value_helper {
      using iterator = I;
      using projection = Proj;
    };
  };
};

} // namespace detail

/**
 * @brief A variation of `std::projected` that accepts async and regular function.
 */
template <std::indirectly_readable I, indirectly_regular_unary_invocable<I> Proj>
using projected = typename detail::projected_impl<I, Proj>::adl_barrier;

// // ---------------------------------- Semigroup  helpers ---------------------------------- //

namespace impl {

/**
 * @brief Verify `F` is invocable with `Args...` and returns `R`.
 */
template <typename R, typename F, typename... Args>
concept regular_invocable_returns =
    regular_invocable<F, Args...> && std::same_as<R, invoke_result_t<F, Args...>>;

} // namespace impl

namespace detail {

/**
 * @brief Test if `Bop` is invocable with all combinations of `T` and `R` and all invocations return `R`.
 */
template <class R, class Bop, class T>
concept semigroup_impl = impl::regular_invocable_returns<R, Bop, T, T> && //
                         impl::regular_invocable_returns<R, Bop, T, R> && //
                         impl::regular_invocable_returns<R, Bop, R, T> && //
                         impl::regular_invocable_returns<R, Bop, R, R>;   //

} // namespace detail

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
concept semigroup =
    regular_invocable<Bop, T, T> && detail::semigroup_impl<invoke_result_t<Bop, T, T>, Bop, T>; //

/**
 * @brief The result of invoking a semigroup's binary operator with two values of type `T`.
 */
template <class Bop, class T>
  requires semigroup<Bop, T>
using semigroup_t = invoke_result_t<Bop, T, T>;

/**
 * @brief Test if a binary operator is a semigroup over `T` and `U` with the same result type.
 *
 * A dual semigroup requires that `Bop` is a semigroup over `T` and `U` with the same
 * `lf::semigroup_t` and mixed invocation of `Bop` over `T` and `U` has semigroup
 * semantics.
 *
 * Let u be an object of type `U` and t be an object of type `T`, the additional following
 * expressions must be valid and return the same `lf::semigroup_t` as the previous expressions:
 *
 * 1. `bop(t, u)`
 * 2. `bop(u, t)`
 *
 * This is commutative in `T` and `U`.
 */
template <class Bop, class T, class U>
concept common_semigroup = semigroup<Bop, T> &&                                               //
                           semigroup<Bop, U> &&                                               //
                           std::same_as<semigroup_t<Bop, T>, semigroup_t<Bop, U>> &&          //
                           impl::regular_invocable_returns<semigroup_t<Bop, T>, Bop, U, T> && //
                           impl::regular_invocable_returns<semigroup_t<Bop, U>, Bop, T, U>;   //

// ------------------------------------ Foldable ------------------------------------ //

namespace detail {

template <class Acc, class Bop, class T>
concept foldable_impl =                               //
    common_semigroup<Bop, Acc, T> &&                  //
    std::movable<Acc> &&                              // Accumulator moved in loop.
    std::convertible_to<semigroup_t<Bop, T>, Acc> &&  // `fold bop [a] = a`
    std::assignable_from<Acc &, semigroup_t<Bop, T>>; // Accumulator assigned in loop.

} // namespace detail

/**
 * @brief Test if a binary operation supports a fold operation over a type.
 *
 * This means a collection of one or more values of type `T` can be folded to a single value
 * of type `Acc` equal to `std::decay_t<semigroup_t<Bop, T>>` using `bop`, an operator of type `Bop`.
 *
 * For example using the infix notation `a · b` to mean `bop(a, b)`:
 *
 * 1. `fold bop [a] = a`
 * 2. `fold bop [a, b] = a · b`
 * 3. `fold bop [a, b, c] = a · b · c`
 * 4. `fold bop [a, b, c, ...] = a · b · c · ...`
 *
 * The order of evaluation is unspecified but the elements will not be reordered.
 */
template <class Bop, class T>
concept foldable =                                                    //
    semigroup<Bop, T> &&                                              //
    detail::foldable_impl<std::decay_t<semigroup_t<Bop, T>>, Bop, T>; //

/**
 * @brief An indirect version of `lf::foldable`.
 */
template <class Bop, class I>
concept indirectly_foldable =                                                 //
    std::indirectly_readable<I> &&                                            //
    std::copy_constructible<Bop> &&                                           //
    common_semigroup<Bop &, indirect_value_t<I>, std::iter_reference_t<I>> && //
    foldable<Bop &, indirect_value_t<I>> &&                                   //
    foldable<Bop &, std::iter_reference_t<I>>;                                //

} // namespace lf

#endif /* D336C448_D1EE_4616_9277_E0D7D550A10A */
