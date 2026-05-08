export module libfork.core:projected;

import std;

import :concepts_invocable;
import :concepts_context;
import :concepts_indirect;

namespace lf {

template <typename I>
struct conditional_difference_type : indirect_value_customization {};

template <std::weakly_incrementable I>
struct conditional_difference_type<I> : indirect_value_customization {
  using difference_type = std::iter_difference_t<I>;
};

// C++26 ADL firewalled implementation.
template <typename I, typename Fn, typename Context>
struct projected_impl;

// sync
template <typename I, typename Fn, typename>
struct projected_impl {
  struct type : conditional_difference_type<I> {
   private:
    friend struct indirect_value<type>;
    using indirect_value_type = std::invoke_result_t<Fn &, indirect_value_t<I>>;

    using reference_type = std::invoke_result_t<Fn &, std::iter_reference_t<I>>;

   public:
    using value_type = std::remove_cvref_t<reference_type>;
    auto operator*() const -> reference_type;
  };
};

// async
template <typename I, typename Fn, typename Context>
  requires indirectly_async_unary_invocable<Fn, Context, I>
struct projected_impl<I, Fn, Context> {
  struct type : conditional_difference_type<I> {
   private:
    friend struct indirect_value<type>;
    using indirect_value_type = async_result_t<Fn &, Context, indirect_value_t<I>>;

    using reference_type = async_result_t<Fn &, Context, std::iter_reference_t<I>>;

   public:
    using value_type = std::remove_cvref_t<reference_type>;
    auto operator*() const -> reference_type;
  };
};

template <typename Fn, typename Context, typename... T>
concept async_defaultable =
    ((async_invocable<Fn, Context, T> && std::default_initializable<async_result_t<Fn, Context, T>>) && ...);

template <typename Fn, typename Context, typename I>
concept async_defaultable_impl =
    async_defaultable<Fn, Context, std::iter_reference_t<I>, indirect_value_t<I>>;

template <typename Fn, typename Context, typename I>
concept indirect_async_defaultable =
    !indirectly_async_regular_unary_invocable<Fn, Context, I> || async_defaultable_impl<Fn, Context, I>;

/**
 * @brief Test if `I` can be projected through `Fn` in the context of `Context`
 *
 * This requires the standard indirectly regular invocable and, in addition,
 * async projections must return a default initializable type.
 *
 * Projectable && indirectly_async_unary_invocable implies an async projection
 * with default initializable async result type.
 */
export template <typename Fn, typename Context, typename I>
concept projectable =
    indirectly_regular_unary_invocable<Fn, Context, I> && indirect_async_defaultable<Fn, Context, I>;

/**
 * @brief A version of `std::projected` that supports both regular invocables and async invocables.
 */
export template <worker_context Context, std::indirectly_readable I, projectable<Context, I> Fn>
using projected = projected_impl<I, Fn, Context>::type;

} // namespace lf
