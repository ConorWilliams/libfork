
export module libfork.core:concepts_indirect;

import std;

import libfork.utils;

import :concepts_invocable;

/**
 * The purpose of this file is to define async versions of:
 *
 *  `indirectly_unary_invocable`
 *
 * This requires `indirect-value-t` which is in turn requires an async version
 * of `projected` which in turns requires an async version of
 * `indirect_result_t`
 *
 */

namespace lf {

// ========= Forward decl =========

template <typename T>
struct indirect_value {
  using type = std::iter_value_t<T> &;
};

template <typename T>
using indirect_value_t = indirect_value<T>::type;

// ========= Core concepts =========

export template <typename Fn, typename Context, typename I>
concept indirectly_unary_async_invocable =                      //
    worker_context<Context> &&                                  //
    std::indirectly_readable<I> &&                              //
    std::copy_constructible<Fn> &&                              //
    async_invocable<Fn &, Context, indirect_value_t<I>> &&      //
    async_invocable<Fn &, Context, std::iter_reference_t<I>> && //
    std::common_reference_with<                                 //
        async_result_t<Fn &, Context, indirect_value_t<I>>,     //
        async_result_t<Fn &, Context, std::iter_reference_t<I>> //
        >;                                                      //

export template <typename Fn, typename Context, typename I>
concept indirectly_regular_unary_async_invocable =
    worker_context<Context> &&                                          //
    std::indirectly_readable<I> &&                                      //
    std::copy_constructible<Fn> &&                                      //
    async_regular_invocable<Fn &, Context, indirect_value_t<I>> &&      //
    async_regular_invocable<Fn &, Context, std::iter_reference_t<I>> && //
    std::common_reference_with<                                         //
        async_result_t<Fn &, Context, indirect_value_t<I>>,             //
        async_result_t<Fn &, Context, std::iter_reference_t<I>>         //
        >;                                                              //

export template <typename Fn, typename Context, typename I>
concept indirectly_unary_invocable =
    indirectly_unary_async_invocable<Fn, Context, I> || std::indirectly_unary_invocable<Fn, I>;

export template <typename Fn, typename Context, typename I>
concept indirectly_regular_unary_invocable = indirectly_regular_unary_async_invocable<Fn, Context, I> ||
                                             std::indirectly_regular_unary_invocable<Fn, I>;

// ========= indirect_result =========

/**
 * @brief A version of `std::invoke_result` that supports both regular invocables and async invocables.
 *
 * This gives preference to async invocation.
 */
template <typename Proj, typename Context, typename... Args>
struct invoke_result : std::invoke_result<Proj, Args...> {};

// More constrained so should be selected if both regular and async invocations are possible.
template <typename Proj, typename Context, typename... Args>
  requires async_invocable<Proj, Context, Args...>
struct invoke_result<Proj, Context, Args...> {
  using type = async_result_t<Proj, Context, Args...>;
};

template <class F, typename Context, typename... Args>
using invoke_result_t = invoke_result<F, Context, Args...>::type;

template <class F, typename Context, std::indirectly_readable... Is>
using indirect_result_t = invoke_result<F, Context, std::iter_reference_t<Is>...>::type;

// ========= Projected =========

struct hidden_projected_base {};

template <bool WeaklyIncrementable, typename I, typename Proj, typename Context>
struct projected_impl {
  struct type : hidden_projected_base {

    // Used by indirect_value
    using hidden_indirect_value = invoke_result<Proj &, Context, indirect_value_t<I>>;

    using value_type = std::remove_cvref_t<indirect_result_t<Proj &, Context, I>>;
    auto operator*() const -> indirect_result_t<Proj &, Context, I>;
  };
};

template <std::weakly_incrementable I, typename Proj, typename Context>
struct projected_impl<true, I, Proj, Context> {
  struct type : projected_impl<false, I, Proj, Context>::type {
    using difference_type = std::iter_difference_t<I>;
  };
};

/**
 * @brief A version of `std::projected` that supports both regular invocables and async invocables.
 */
export template <std::indirectly_readable I, typename Proj, worker_context Context>
  requires indirectly_regular_unary_invocable<Proj, Context, I>
using projected = projected_impl<std::weakly_incrementable<I>, I, Proj, Context>::type;

// Specialization of indirect_value

template <typename P>
  requires std::derived_from<P, hidden_projected_base>
struct indirect_value<P> : P::hidden_indirect_value {};

} // namespace lf
