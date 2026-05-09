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
  using iter_difference_t = std::iter_difference_t<T>;

  template <typename T>
  using range_difference_t = std::ranges::range_difference_t<T>;

 public:
  // (1) iterator-pair, chunk size n >= 1
  template <worker_context Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename T,
            typename Bop,
            projectable<Context, I> Proj = std::identity>
    requires indirectly_foldable<Bop, Context, projected<Context, I, Proj>>
  static auto operator()(
      env<Context> /* env */, I head, S tail, iter_difference_t<I> n, T identity, Bop bop, Proj proj = {})
      -> lf::task<T, Context> {

    LF_ASSUME(n > 0);

    auto len = tail - head;

    LF_ASSUME(len >= 0);

    if (len <= n) {
      // Leaf: sequential fold over chunk, seeded with `identity`.
      auto sc = co_await scope();
      T acc = std::move(identity);

      for (; head != tail; ++head) {
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
      }
      co_return acc;
    }

    // Split. Storage is copy-init'd from `identity` (avoids a default_initializable<T>
    // requirement); `*return_addr = result` in the child's promise overwrites it.
    auto mid = head + (len / 2);
    auto sc = co_await scope();
    T lhs(identity);
    T rhs(identity);
    co_await sc.fork(&lhs, fold_impl{}, head, mid, n, identity, bop, proj);
    co_await sc.call(&rhs, fold_impl{}, mid, tail, n, identity, std::move(bop), std::move(proj));
    co_await sc.join();

    if constexpr (async_invocable<Bop &, Context, T, T>) {
      // TODO: can this be a tail call / use current return address?
      co_await sc.call(&lhs, bop, std::move(lhs), std::move(rhs));
      co_await sc.join();
    } else {
      lhs = std::invoke(bop, std::move(lhs), std::move(rhs));
    }
    co_return lhs;
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
  static auto operator()(
      env<Context> context, Range &&range, range_difference_t<Range> n, T identity, Bop bop, Proj proj = {})
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
