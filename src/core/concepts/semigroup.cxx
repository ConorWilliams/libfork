export module libfork.core:concepts_semigroup;

import std;

import :concepts_invocable;
import :concepts_indirect;

namespace lf {

// === Semigroup

namespace sync {

template <typename R, typename Fn, typename... Args>
concept invocable_to = std::invocable<Fn, Args...> && std::same_as<std::invoke_result_t<Fn, Args...>, R>;

template <typename R, typename Fn, typename T>
concept semigroup_r =                //
    std::constructible_from<R, T> && //
    invocable_to<R, Fn, T, T> &&     //
    invocable_to<R, Fn, T, R> &&     //
    invocable_to<R, Fn, R, T> &&     //
    invocable_to<R, Fn, R, R>;       //

template <typename R, typename Fn, typename I>
concept indirect_semigroup_r =                                              //
    semigroup_r<R, Fn &, indirect_value_t<I>> &&                            //
    semigroup_r<R, Fn &, std::iter_reference_t<I>> &&                       //
    invocable_to<R, Fn &, indirect_value_t<I>, std::iter_reference_t<I>> && //
    invocable_to<R, Fn &, std::iter_reference_t<I>, indirect_value_t<I>>;   //

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
 * Let `t`, `u` and `f` be objects of types `T`, `U` and `Fn` respectively.
 * Then the following expression must be valid:
 *
 * ```
 * f(u, t)
 * ```
 *
 * And return the same type `R` for all combinations of `T` and `U` being `R`,
 * `indirect_value_t<I>` and `std::iter_reference_t<I>`.
 */
export template <typename Fn, typename I>
concept indirect_semigroup =                                                  //
    std::indirectly_readable<I> &&                                            //
    std::copy_constructible<Fn> &&                                            //
    std::regular_invocable<Fn &, indirect_value_t<I>, indirect_value_t<I>> && //
    indirect_semigroup_r<                                                     //
        std::invoke_result_t<Fn &, indirect_value_t<I>, indirect_value_t<I>>, //
        Fn,                                                                   //
        I                                                                     //
        >;                                                                    //

} // namespace sync

namespace async {

template <typename R, typename Fn, typename Context, typename T>
concept semigroup_r =                           //
    std::constructible_from<R, T> &&            //
    async_invocable_to<Fn, R, Context, T, T> && //
    async_invocable_to<Fn, R, Context, T, R> && //
    async_invocable_to<Fn, R, Context, R, T> && //
    async_invocable_to<Fn, R, Context, R, R>;   //

template <typename R, typename Fn, typename Context, typename I>
concept indirect_semigroup_r =                                                             //
    semigroup_r<R, Fn &, Context, indirect_value_t<I>> &&                                  //
    semigroup_r<R, Fn &, Context, std::iter_reference_t<I>> &&                             //
    async_invocable_to<Fn &, R, Context, indirect_value_t<I>, std::iter_reference_t<I>> && //
    async_invocable_to<Fn &, R, Context, std::iter_reference_t<I>, indirect_value_t<I>>;   //

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
 * Let `t`, `u` and `f` be objects of types `T`, `U` and `Fn` respectively.
 * Then the following expression must be valid:
 *
 * ```
 * R ret;
 * co_await scope.call(std::addressof(ret), f, u, t)
 * ```
 *
 * And return the same type `R` for all combinations of `T` and `U` being `R`,
 * `indirect_value_t<I>` and `std::iter_reference_t<I>`.
 */
export template <typename Fn, typename Context, typename I>
concept indirect_semigroup =                                                     //
    std::indirectly_readable<I> &&                                               //
    worker_context<Context> &&                                                   //
    std::copy_constructible<Fn> &&                                               //
    async_invocable<Fn &, Context, indirect_value_t<I>, indirect_value_t<I>> &&  //
    indirect_semigroup_r<                                                        //
        async_result_t<Fn &, Context, indirect_value_t<I>, indirect_value_t<I>>, //
        Fn,                                                                      //
        Context,                                                                 //
        I                                                                        //
        >;                                                                       //

} // namespace async

/**
 * @brief Either a synchronous or asynchronous semigroup.
 */
export template <typename Fn, typename Context, typename I>
concept indirect_semigroup = async::indirect_semigroup<Fn, Context, I> || sync::indirect_semigroup<Fn, I>;

/**
 * @brief A semantic requirement that the semigroup operation is commutative.
 *
 * Commutativity requires `a · b = b · a` for all `a`, `b` in the set `S`.
 */
export template <typename Fn, typename Context, typename I>
concept indirect_commutative_semigroup = indirect_semigroup<Fn, Context, I>;

template <typename Fn, typename Context, typename I>
struct indirect_semigroup_result {
  using type = std::invoke_result_t<Fn &, indirect_value_t<I>, indirect_value_t<I>>;
};

template <typename Fn, typename Context, typename I>
  requires async::indirect_semigroup<Fn, Context, I>
struct indirect_semigroup_result<Fn, Context, I> {
  using type = async_result_t<Fn &, Context, indirect_value_t<I>, indirect_value_t<I>>;
};

/**
 * @brief Get the result type of an indirect semigroup operation.
 *
 * This is the type of the result of applying the semigroup operation to two elements of the set.
 */
export template <typename Fn, typename Context, typename I>
  requires indirect_semigroup<Fn, Context, I>
using indirect_semigroup_t = indirect_semigroup_result<Fn, Context, I>::type;

} // namespace lf
