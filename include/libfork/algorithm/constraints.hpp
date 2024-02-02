#ifndef D336C448_D1EE_4616_9277_E0D7D550A10A
#define D336C448_D1EE_4616_9277_E0D7D550A10A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for copy_constructible, same_as, common_reference_with
#include <iterator>    // for indirectly_readable, iter_reference_t, indirectly_...
#include <type_traits> // for decay_t, false_type, invoke_result, remove_cvref_t

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
  requires requires { typename Proj::secret_projected_indirect_value; }
struct indirect_value_impl<Proj> : Proj::secret_projected_indirect_value {};

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
  /**
   * @brief An ADL barrier.
   */
  struct projected_iterator : conditional_difference_type<I> {
    /**
     * @brief The value_type of the projected iterator.
     */
    using value_type = std::remove_cvref_t<indirect_result_t<Proj &, I>>;
    /**
     * @brief Not defined.
     */
    auto operator*() const -> indirect_result_t<Proj &, I>; ///<
    /**
     * @brief For internal use only!
     */
    struct secret_projected_indirect_value {
      using type = invoke_result_t<Proj &, indirect_value_t<I>>;
    };
  };
};

/**
 * @brief A variation of `std::projected` that accepts async/regular function.
 */
template <std::indirectly_readable I, indirectly_regular_unary_invocable<I> Proj>
using project_once = typename detail::projected_impl<I, Proj>::projected_iterator;

template <typename...>
struct compose_projection {};

template <std::indirectly_readable I>
struct compose_projection<I> : std::type_identity<I> {};

template <std::indirectly_readable I, indirectly_regular_unary_invocable<I> Proj, typename... Other>
struct compose_projection<I, Proj, Other...> : compose_projection<project_once<I, Proj>, Other...> {};

//

template <typename...>
struct composable : std::false_type {};

template <std::indirectly_readable I>
struct composable<I> : std::true_type {};

template <std::indirectly_readable I, indirectly_regular_unary_invocable<I> Proj, typename... Other>
struct composable<I, Proj, Other...> : composable<project_once<I, Proj>, Other...> {};

template <typename I, typename... Proj>
concept indirectly_composable = std::indirectly_readable<I> && composable<I, Proj...>::value;

} // namespace detail

/**
 * @brief A variation of `std::projected` that accepts async/regular functions and composes projections.
 */
template <std::indirectly_readable I, typename... Proj>
  requires detail::indirectly_composable<I, Proj...>
using projected = typename detail::compose_projection<I, Proj...>::type;

// Quick test

static_assert(std::same_as<indirect_value_t<int *>, int &>);
static_assert(std::same_as<indirect_value_t<projected<int *, int (*)(int &)>>, int>);

// ---------------------------------- Semigroup  helpers ---------------------------------- //

namespace impl {

/**
 * @brief Verify `F` is invocable with `Args...` and returns `R`.
 */
template <typename R, typename F, typename... Args>
concept regular_invocable_returns =
    regular_invocable<F, Args...> && std::same_as<R, invoke_result_t<F, Args...>>;

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
 * Let `t`, `u` and `bop` be objects of types `T`, `U` and `Bop` respectively. Then the following expressions
 * must be valid:
 *
 * 1. `bop(t, t)`
 * 2. `bop(u, u)`
 * 3. `bop(u, t)`
 * 4. `bop(t, u)`
 *
 * Additionally, the expressions must return the same type, `R`.
 *
 * __Note:__ A semigroup requires all invocations to be regular. This is a semantic requirement only.
 */
template <class Bop, class T, class U>
concept semigroup =
    regular_invocable<Bop, T, T> &&                                           // Pure invocations
    regular_invocable<Bop, U, U> &&                                           //
    std::same_as<invoke_result_t<Bop, T, T>, invoke_result_t<Bop, U, U>> &&   //
    impl::regular_invocable_returns<invoke_result_t<Bop, T, T>, Bop, T, U> && // Mixed invocations
    impl::regular_invocable_returns<invoke_result_t<Bop, U, U>, Bop, U, T>;   //

// ------------------------------------ Foldable ------------------------------------ //

namespace detail {

template <class Acc, class Bop, class T>
concept foldable_to =                                        //
    std::movable<Acc> &&                                     //
    semigroup<Bop, Acc, T> &&                                //
    std::constructible_from<Acc, T> &&                       //
    std::convertible_to<T, Acc> &&                           //
    std::assignable_from<Acc &, invoke_result_t<Bop, T, T>>; //

template <class Acc, class Bop, class I>
concept indirectly_foldable_to =                                       //
    std::indirectly_readable<I> &&                                     //
    std::copy_constructible<Bop> &&                                    //
    semigroup<Bop &, indirect_value_t<I>, std::iter_reference_t<I>> && //
    foldable_to<Acc, Bop &, indirect_value_t<I>> &&                    //
    foldable_to<Acc, Bop &, std::iter_reference_t<I>>;                 //

} // namespace detail

/**
 * @brief Test if a binary operation supports a fold operation over a type.
 *
 * The binary operation must be associative but not necessarily commutative.
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
 *
 * @tparam Bop Associative binary operator.
 * @tparam I Input type
 */
template <class Bop, class T>
concept foldable =                                                         //
    invocable<Bop, T, T> &&                                                //
    detail::foldable_to<std::decay_t<invoke_result_t<Bop, T, T>>, Bop, T>; //

/**
 * @brief An indirect version of `lf::foldable`.
 *
 * @tparam Bop Associative binary operator.
 * @tparam I Input iterator.
 */
template <class Bop, class I>
concept indirectly_foldable =                                                             //
    std::indirectly_readable<I> &&                                                        //
    std::copy_constructible<Bop> &&                                                       //
    invocable<Bop &, std::iter_reference_t<I>, std::iter_reference_t<I>> &&               //
    detail::indirectly_foldable_to<std::decay_t<indirect_result_t<Bop &, I, I>>, Bop, I>; //

/**
 * @brief Compute the accumulator/result type for a fold operation.
 */
template <class Bop, std::random_access_iterator I, class Proj>
  requires indirectly_foldable<Bop, projected<I, Proj>>
using indirect_fold_acc_t = std::decay_t<indirect_result_t<Bop &, projected<I, Proj>, projected<I, Proj>>>;

namespace detail {

template <class Acc, class Bop, class O>
concept scannable_impl =
    std::indirectly_writable<O, Acc> &&                                            // Write result of fold.
    semigroup<Bop &, std::iter_reference_t<O>, std::iter_rvalue_reference_t<O>> && // Scan/reduce over O.
    std::indirectly_writable<O, indirect_result_t<Bop &, O, O>> &&                 //   Write result of -^.
    std::indirectly_writable<O, Acc &> &&                                          // Copy acc in scan.
    std::constructible_from<Acc, std::iter_reference_t<O>> &&                      // Initialize acc in scan.
    std::convertible_to<std::iter_reference_t<O>, Acc>;                            // Same as -^.

}

template <class Bop, class O, class T>
concept scannable =                                       //
    std::indirectly_readable<O> &&                        //
    std::indirectly_writable<O, T> &&                     // n = 1 case.
    detail::foldable_to<std::iter_value_t<O>, Bop, T> &&  // Regular reduction over T.
    detail::scannable_impl<std::iter_value_t<O>, Bop, O>; //

// TODO requirements for last one once we fix such that the last element is used.

template <class Bop, class O, class I>
concept indirectly_scannable =
    std::indirectly_readable<O> &&                                  //
    std::indirectly_readable<I> &&                                  //
    std::copy_constructible<Bop> &&                                 //
    std::indirectly_writable<O, indirect_value_t<I>> &&             // n = 1 case.
    std::indirectly_writable<O, std::iter_reference_t<I>> &&        //   -^.
    detail::indirectly_foldable_to<std::iter_value_t<O>, Bop, I> && // Regular reduction over T.
    detail::scannable_impl<std::iter_value_t<O>, Bop, O>;

} // namespace lf

#endif /* D336C448_D1EE_4616_9277_E0D7D550A10A */
