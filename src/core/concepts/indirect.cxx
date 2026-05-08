export module libfork.core:concepts_indirect;

import std;

import :concepts_invocable;
import :concepts_context;

/**
 * The purpose of this file is to define async versions of:
 *
 *  `indirectly_unary_invocable`
 *
 * This requires `indirect-value-t` which is in turn requires an async version of `projected`.
 */

namespace lf {

// ========= Forward decl =========

/*

TODO: consolodate this notse into comments

With indirect_value_t the constraint for projections:
iter_value_t<It> x = *it;
f(proj(x));

Without indirect_value_t the constraint is
iter_value_t<projected<I,Fn>> u = proj(*it);
f(u);

i.e. indirect_value_t is value_type<I> & for normal or proj(value_type<I> &) for projected.
*/

template <typename I>
struct indirect_value {
  using type = std::iter_value_t<I> &;
};

// A type can derive from this to opt-into indirect-value-t customization
struct indirect_value_customization {};

// Specialization of indirect_value
template <std::derived_from<indirect_value_customization> T>
struct indirect_value<T> {
  using type = T::indirect_value_type;
};

template <typename I>
using indirect_value_t = indirect_value<I>::type;

// ========= Core concepts =========

// We must duplicate the std:: versions to work with our version of projected

export template <typename Fn, typename I>
concept indirectly_sync_unary_invocable =                    //
    std::indirectly_readable<I> &&                           //
    std::copy_constructible<Fn> &&                           //
    std::invocable<Fn &, indirect_value_t<I>> &&             //
    std::invocable<Fn &, std::iter_reference_t<I>> &&        //
    std::common_reference_with<                              //
        std::invoke_result_t<Fn &, indirect_value_t<I>>,     //
        std::invoke_result_t<Fn &, std::iter_reference_t<I>> //
        >;                                                   //

export template <typename Fn, typename I>
concept indirectly_sync_regular_unary_invocable =             //
    std::indirectly_readable<I> &&                            //
    std::copy_constructible<Fn> &&                            //
    std::regular_invocable<Fn &, indirect_value_t<I>> &&      //
    std::regular_invocable<Fn &, std::iter_reference_t<I>> && //
    std::common_reference_with<                               //
        std::invoke_result_t<Fn &, indirect_value_t<I>>,      //
        std::invoke_result_t<Fn &, std::iter_reference_t<I>>  //
        >;                                                    //

// TODO: Document

export template <typename Fn, typename Context, typename I>
concept indirectly_async_unary_invocable =                      //
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
concept indirectly_async_regular_unary_invocable =
    worker_context<Context> &&                                          //
    std::indirectly_readable<I> &&                                      //
    std::copy_constructible<Fn> &&                                      //
    async_regular_invocable<Fn &, Context, indirect_value_t<I>> &&      //
    async_regular_invocable<Fn &, Context, std::iter_reference_t<I>> && //
    std::common_reference_with<                                         //
        async_result_t<Fn &, Context, indirect_value_t<I>>,             //
        async_result_t<Fn &, Context, std::iter_reference_t<I>>         //
        >;                                                              //

/**
 * @brief TODO:desc
 *
 * In general if a function is both sync and async invocable it is expected that
 */
export template <typename Fn, typename Context, typename I>
concept indirectly_unary_invocable =
    indirectly_async_unary_invocable<Fn, Context, I> || indirectly_sync_unary_invocable<Fn, I>;

// clang-format off

export template <typename Fn, typename Context, typename I>
concept indirectly_regular_unary_invocable = 
    indirectly_async_regular_unary_invocable<Fn, Context, I> || indirectly_sync_regular_unary_invocable<Fn, I>;

// clang-format on

} // namespace lf
