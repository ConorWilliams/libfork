module;
#include "libfork/__impl/assume.hpp"
export module libfork.algorithm:for_each;

import std;

import libfork.core;

namespace lf {

template <typename T>
concept sized_random_access_range = std::ranges::random_access_range<T> && std::ranges::sized_range<T>;

template <typename Fn, typename Context, typename I>
concept iter_indirect_unary_invocable =
    std::indirectly_unary_invocable<Fn, I> ||
    async_invocable<Fn, Context, std::iter_reference_t<I>>;

struct for_each_impl {
 private:
  template <worker_context Context>
  using task = lf::task<void, Context>;

  template <typename T>
  using iter_difference_t = std::iter_difference_t<T>;

  template <typename T>
  using range_difference_t = std::ranges::range_difference_t<T>;

 public:
  // (1) iterator-pair, chunk size n > 1
  template <worker_context Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Fn>
    requires iter_indirect_unary_invocable<Fn, Context, I>
  static auto
  operator()(env<Context> /* env */, I head, S tail, iter_difference_t<I> n, Fn fn) -> task<Context> {

    LF_ASSUME(n > 0);

    auto len = tail - head;

    LF_ASSUME(len >= 0);

    if (len <= n) {
      if constexpr (std::indirectly_unary_invocable<Fn, I>) {
        for (; head != tail; ++head) {
          std::invoke(fn, *head);
        }
      } else {
        auto sc = co_await scope();
        for (; head != tail; ++head) {
          co_await sc.call_drop(fn, *head);
        }
        co_await sc.join();
      }
      co_return;
    }

    auto mid = head + (len / 2);
    auto sc = co_await scope();
    co_await sc.fork(for_each_impl{}, head, mid, n, fn);
    co_await sc.call(for_each_impl{}, mid, tail, n, std::move(fn));
    co_await sc.join();
  }

  // (2) iterator-pair, n == 1 specialization (no n parameter)
  template <worker_context Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Fn>
    requires iter_indirect_unary_invocable<Fn, Context, I>
  static auto operator()(env<Context> /* env */, I head, S tail, Fn fn) -> task<Context> {

    auto len = tail - head;

    LF_ASSUME(len >= 0);

    switch (len) {
      case 0:
        co_return;
      case 1:
        if constexpr (std::indirectly_unary_invocable<Fn, I>) {
          std::invoke(fn, *head);
        } else {
          auto sc = co_await scope();
          co_await sc.call_drop(std::move(fn), *head);
          co_await sc.join();
        }
        co_return;
    }

    auto mid = head + (len / 2);
    auto sc = co_await scope();
    co_await sc.fork(for_each_impl{}, head, mid, fn);
    co_await sc.call(for_each_impl{}, mid, tail, std::move(fn));
    co_await sc.join();
  }

  // (3) range + n -> dispatches to (1) or (2)
  template <worker_context Context,
            sized_random_access_range Range,
            typename Fn>
    requires iter_indirect_unary_invocable<Fn, Context, std::ranges::iterator_t<Range>>
  static auto
  operator()(env<Context> context, Range &&range, range_difference_t<Range> n, Fn fn) -> task<Context> {
    if (n == 1) {
      return for_each_impl{}(context, std::ranges::begin(range), std::ranges::end(range), std::move(fn));
    }
    return for_each_impl{}(context, std::ranges::begin(range), std::ranges::end(range), n, std::move(fn));
  }

  // (4) range, n == 1 -> dispatches to (2)
  template <worker_context Context,
            sized_random_access_range Range,
            typename Fn>
    requires iter_indirect_unary_invocable<Fn, Context, std::ranges::iterator_t<Range>>
  static auto operator()(env<Context> context, Range &&range, Fn fn) -> task<Context> {
    return for_each_impl{}(context, std::ranges::begin(range), std::ranges::end(range), std::move(fn));
  }
};

export inline constexpr for_each_impl for_each = {};

} // namespace lf
