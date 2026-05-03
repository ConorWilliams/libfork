module;
#include <iterator>
export module libfork.algorithm:projected;

import std;

import libfork.core;

namespace lf {

// ============================================================================
//  projection_result
// ----------------------------------------------------------------------------
//  The reference type produced when projecting an iterator value through a
//  projection. Two cases:
//   - async: Proj is async_invocable on iter_reference_t<I>; the projection's
//            "result" is async_result_t (the value_type of the returned task).
//   - sync : Proj is a regular invocable; the result is invoke_result_t,
//            matching std::indirect_result_t — the same as std::projected.
// ============================================================================

template <typename Proj, typename Context, typename I>
struct projection_result;

template <typename Proj, typename Context, typename I>
  requires async_invocable<Proj &, Context, std::iter_reference_t<I>>
struct projection_result<Proj, Context, I> {
  using type = async_result_t<Proj &, Context, std::iter_reference_t<I>>;
};

template <typename Proj, typename Context, typename I>
  requires (!async_invocable<Proj &, Context, std::iter_reference_t<I>>) &&
           std::regular_invocable<Proj &, std::iter_reference_t<I>>
struct projection_result<Proj, Context, I> {
  using type = std::invoke_result_t<Proj &, std::iter_reference_t<I>>;
};

template <typename Proj, typename Context, typename I>
using projection_result_t = typename projection_result<Proj, Context, I>::type;

// ============================================================================
//  Concepts: indirectly invocable, optionally async.
// ----------------------------------------------------------------------------
//  Mirror std::indirectly_(regular_)?unary_invocable but accept either a
//  regular invocable or an async_invocable (returning a task). The
//  std::common_reference_with leg of the std concepts is dropped in the async
//  branch — there is no ordering between two distinct task value_types beyond
//  what async_result_t already exposes, and propagating common-reference
//  constraints across task boundaries is not meaningful.
// ============================================================================

export template <typename Proj, typename Context, typename I>
concept indirectly_async_or_regular_unary_invocable =
    worker_context<Context> &&                                    //
    std::indirectly_readable<I> &&                                //
    std::copy_constructible<Proj> &&                              //
    (std::indirectly_regular_unary_invocable<Proj, I> ||          //
     async_invocable<Proj &, Context, std::iter_reference_t<I>>); //

export template <typename Fn, typename Context, typename I>
concept indirectly_async_or_unary_invocable = worker_context<Context> &&                                  //
                                              std::indirectly_readable<I> &&                              //
                                              std::copy_constructible<Fn> &&                              //
                                              (std::indirectly_unary_invocable<Fn, I> ||                  //
                                               async_invocable<Fn &, Context, std::iter_reference_t<I>>); //

// ============================================================================
//  lf::projected — libfork's analogue of std::projected (C++26 form).
// ----------------------------------------------------------------------------
//  Combines an indirectly_readable type I and a projection Proj into a new
//  indirectly_readable type whose dereference yields the projected reference.
//  When Proj is an async_invocable, the dereference type is the value_type of
//  the task returned by the projection — i.e. what the algorithm sees after
//  awaiting. When Proj is a plain invocable, this matches std::projected.
//
//  Like std::projected, the alias is implemented via an indirect layer
//  (projected_impl<I, Proj, Context>::type) so I, Proj and Context never
//  become associated classes of the projected type.
//
//  operator* is declared but not defined — projected is for constraint
//  checking only and never has a runtime instance.
// ============================================================================

template <bool WeaklyIncrementable, typename I, typename Proj, typename Context>
struct projected_impl {
  struct type {
    using value_type = std::remove_cvref_t<projection_result_t<Proj, Context, I>>;
    auto operator*() const -> projection_result_t<Proj, Context, I>;
  };
};

template <std::weakly_incrementable I, typename Proj, typename Context>
struct projected_impl<true, I, Proj, Context> {
  struct type : projected_impl<false, I, Proj, Context>::type {
    using difference_type = std::iter_difference_t<I>;
  };
};

export template <std::indirectly_readable I, typename Proj, worker_context Context>
  requires indirectly_async_or_regular_unary_invocable<Proj, Context, I>
using projected = projected_impl<std::weakly_incrementable<I>, I, Proj, Context>::type;

} // namespace lf
