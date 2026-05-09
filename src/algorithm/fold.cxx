module;
#include "libfork/__impl/assume.hpp"
export module libfork.algorithm:fold;

import std;

import libfork.core;

import :concepts;

namespace lf {

template <typename Fn, typename Context, typename I>
concept indirectly_foldable =
    indirect_semigroup<Fn, Context, I> && std::movable<indirect_semigroup_t<Fn, Context, I>>;

struct fold_impl {
 private:
  template <typename T>
  using iter_diff_t = std::iter_difference_t<T>;

  template <typename T>
  using range_diff_t = std::ranges::range_difference_t<T>;

  template <typename Context, typename I, typename Proj, typename Bop>
  using result_t = lf::indirect_semigroup_t<Bop, Context, projected<Context, I, Proj>>;

  template <typename Context, typename I, typename Proj, typename Bop>
  using task_t = lf::task<result_t<Context, I, Proj, Bop>, Context>;

 public:
  // (1) iterator-pair, chunk size n >= 1
  template <worker_context X,                                 //
            std::random_access_iterator I,                    //
            std::sized_sentinel_for<I> S,                     //
            projectable<X, I> Proj = std::identity,           //
            indirectly_foldable<X, projected<X, I, Proj>> Bop //
            >
  static auto
  operator()(env<X>, I head, S tail, iter_diff_t<I> n, Bop bop, Proj proj = {}) -> task_t<X, I, Proj, Bop> {

    LF_ASSUME(n > 0);

    auto len = tail - head;

    LF_ASSUME(len > 0);

    if (len <= n) {

      if constexpr (async::indirectly_regular_unary_invocable<Proj, X, I>) {

        // TODO: could optimize if this type == accumulator type
        async_result_t<Proj &, X, std::iter_reference_t<I>> init;
        {
          auto sc = co_await scope();
          co_await sc.call(&init, proj, *head);
          co_await sc.join();
        }

        result_t<X, I, Proj, Bop> acc = std::move(init);

        for (++head; head != tail; ++head) {

          async_result_t<Proj &, X, std::iter_reference_t<I>> tmp;
          {
            auto sc = co_await scope();
            co_await sc.call(&init, proj, *head);
            co_await sc.join();
          }

          if constexpr (async::indirect_semigroup<Bop, X, projected<X, I, Proj>>) {
            auto sc = co_await scope();
            co_await sc.call(&acc, bop, std::move(acc), std::move(tmp));
            co_await sc.join();
          } else {
            acc = std::invoke(bop, std::move(acc), std::move(tmp));
          }
        }

      } else {

        result_t<X, I, Proj, Bop> acc = proj(*head);

        for (++head; head != tail; ++head) {
          if constexpr (async::indirect_semigroup<Bop, X, projected<X, I, Proj>>) {
            auto sc = co_await scope();
            co_await sc.call(&acc, bop, std::move(acc), std::invoke(proj, *head));
            co_await sc.join();
          } else {
            acc = std::invoke(bop, std::move(acc), std::invoke(proj, *head));
          }
        }

        co_return acc;
      }
    }

    auto mid = head + (len / 2);

    result_t<X, I, Proj, Bop> lhs;
    result_t<X, I, Proj, Bop> rhs;

    {
      auto sc = co_await scope();
      co_await sc.fork(&lhs, fold_impl{}, head, mid, n, bop, proj);
      co_await sc.call(&rhs, fold_impl{}, mid, tail, n, bop, std::move(proj));
      co_await sc.join();
    }

    if constexpr (async::indirect_semigroup<Bop, X, projected<X, I, Proj>>) {
      // TODO: can this be a tail call / use current return address?
      auto sc = co_await scope();
      co_await sc.call(&lhs, bop, std::move(lhs), std::move(rhs));
      co_await sc.join();
      co_return lhs;
    } else {
      co_return std::invoke(bop, std::move(lhs), std::move(rhs));
    }
  }

  // (2) iterator-pair, n == 1 specialization (no n parameter)
  template <worker_context Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename T,
            typename Bop,
            typename Proj = std::identity>
    requires indirectly_foldable<Bop, Context, projected<Context, I, Proj>>
  static auto operator()(env<Context> /* env */, I head, S tail, T identity, Bop bop, Proj proj = {})
      -> lf::task<T, Context> {

    auto len = tail - head;

    LF_ASSUME(len >= 0);

    switch (len) {
      case 0:
        co_return std::move(identity);
      case 1: {
        auto sc = co_await scope();
        T acc = std::move(identity);
        if constexpr (async::indirectly_regular_unary_invocable<Proj, Context, I>) {
          using PV = typename projected<Context, I, Proj>::value_type;
          PV pv;
          co_await sc.call(&pv, proj, *head);
          co_await sc.join();
          if constexpr (async_invocable<Bop &, Context, T, PV>) {
            co_await sc.call(&acc, bop, std::move(acc), std::move(pv));
            co_await sc.join();
          } else {
            acc = std::invoke(bop, std::move(acc), std::move(pv));
          }
        } else {
          using PV = std::remove_cvref_t<std::invoke_result_t<Proj &, std::iter_reference_t<I>>>;
          if constexpr (async_invocable<Bop &, Context, T, PV>) {
            co_await sc.call(&acc, bop, std::move(acc), std::invoke(proj, *head));
            co_await sc.join();
          } else {
            acc = std::invoke(bop, std::move(acc), std::invoke(proj, *head));
          }
        }
        co_return acc;
      }
    }

    auto mid = head + (len / 2);
    auto sc = co_await scope();
    T lhs(identity);
    T rhs(identity);
    co_await sc.fork(&lhs, fold_impl{}, head, mid, identity, bop, proj);
    co_await sc.call(&rhs, fold_impl{}, mid, tail, identity, std::move(bop), std::move(proj));
    co_await sc.join();

    if constexpr (async_invocable<Bop &, Context, T, T>) {
      co_await sc.call(&lhs, bop, std::move(lhs), std::move(rhs));
      co_await sc.join();
    } else {
      lhs = std::invoke(bop, std::move(lhs), std::move(rhs));
    }
    co_return lhs;
  }

  // (3) range + n -> dispatches to (1) or (2)
  template <worker_context Context,
            sized_random_access_range Range,
            typename T,
            typename Bop,
            typename Proj = std::identity>
    requires indirectly_foldable<Bop, Context, projected<Context, std::ranges::iterator_t<Range>, Proj>>
  static auto
  operator()(env<Context> context, Range &&range, range_diff_t<Range> n, T identity, Bop bop, Proj proj = {})
      -> lf::task<T, Context> {
    if (n == 1) {
      return fold_impl{}(context,
                         std::ranges::begin(range),
                         std::ranges::end(range),
                         std::move(identity),
                         std::move(bop),
                         std::move(proj));
    }
    return fold_impl{}(context,
                       std::ranges::begin(range),
                       std::ranges::end(range),
                       n,
                       std::move(identity),
                       std::move(bop),
                       std::move(proj));
  }

  // (4) range, n == 1 -> dispatches to (2)
  template <worker_context Context,
            sized_random_access_range Range,
            typename T,
            typename Bop,
            typename Proj = std::identity>
    requires indirectly_foldable<Bop, Context, projected<Context, std::ranges::iterator_t<Range>, Proj>>
  static auto operator()(env<Context> context, Range &&range, T identity, Bop bop, Proj proj = {})
      -> lf::task<T, Context> {
    return fold_impl{}(context,
                       std::ranges::begin(range),
                       std::ranges::end(range),
                       std::move(identity),
                       std::move(bop),
                       std::move(proj));
  }
};

export inline constexpr fold_impl fold = {};

} // namespace lf
