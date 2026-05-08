export module libfork.core:concepts_indirect;

import std;

import :concepts_invocable;
import :concepts_context;

namespace lf {

// A type can derive from this to opt-into indirect-value-t customization
struct indirect_value_customization {};

template <typename I>
struct indirect_value {
  using type = std::iter_value_t<I> &;
};

// strip cv-ref qualifiers
template <typename T>
  requires (!std::same_as<T, std::remove_cvref_t<T>>)
struct indirect_value<T> : indirect_value<std::remove_cvref_t<T>> {};

// Specialization for types that customize
template <std::derived_from<indirect_value_customization> T>
  requires std::same_as<T, std::remove_cvref_t<T>>
struct indirect_value<T> {
  using type = T::indirect_value_type;
};

template <typename I>
using indirect_value_t = indirect_value<I>::type;

// ========= Core concepts =========

/**
 * @brief A version of `std::indirectly_unary_invocable` that supports
 * libfork's projection type.
 */
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

/**
 * @brief A version of `std::indirectly_regular_unary_invocable` that supports
 * libfork's projection type.
 */
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

/**
 * @brief A variant of `std::indirectly_unary_invocable` that supports
 * libfork's projection type and requires an async invocable.
 */
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

/**
 * @brief A variant of `std::indirectly_regular_unary_invocable` that supports
 * libfork's projection type and requires an async invocable.
 */
export template <typename Fn, typename Context, typename I>
concept indirectly_async_regular_unary_invocable =                      //
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
 * @brief A variant of `std::indirectly_unary_invocable` that supports either
 * sync or async invocables.
 *
 * In general if a function is both sync and async invocable it is expected
 * that the async version will be preferred.
 */
export template <typename Fn, typename Context, typename I>
concept indirectly_unary_invocable =
    indirectly_async_unary_invocable<Fn, Context, I> || indirectly_sync_unary_invocable<Fn, I>;

// clang-format off

/**
 * @brief A variant of `std::indirectly_regular_unary_invocable` that supports
 * either sync or async invocables.
 *
 * In general if a function is both sync and async invocable it is expected
 * that the async version will be preferred.
 */
export template <typename Fn, typename Context, typename I>
concept indirectly_regular_unary_invocable = 
    indirectly_async_regular_unary_invocable<Fn, Context, I> || indirectly_sync_regular_unary_invocable<Fn, I>;

// clang-format on

} // namespace lf
