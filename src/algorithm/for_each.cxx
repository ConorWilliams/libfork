module;
#include "libfork/__impl/assume.hpp"
export module libfork.algorithm:for_each;

import std;

import libfork.core;

namespace lf {

struct for_each_overload {
 private:
  template <worker_context Context>
  using task = lf::task<void, Context>;

 public:
  // (1) iterator-pair, chunk size n > 1
  template <worker_context Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fn>
  static auto
  operator()(env<Context> /* env */, I head, S tail, std::iter_difference_t<I> n, Fn fn) -> task<Context> {

    LF_ASSUME(n > 0);

    auto len = tail - head;

    LF_ASSUME(len >= 0);

    if (len <= n) {
      for (; head != tail; ++head) {
        std::invoke(fn, *head);
      }
      co_return;
    }

    auto mid = head + (len / 2);
    auto sc = co_await scope();
    co_await sc.fork(for_each_overload{}, head, mid, n, fn);
    co_await sc.call(for_each_overload{}, mid, tail, n, fn);
    co_await sc.join();
  }

  // (2) iterator-pair, n == 1 specialization (no n parameter)
  template <worker_context Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fn>
  static auto operator()(env<Context> /* env */, I head, S tail, Fn fn) -> task<Context> {

    auto len = tail - head;

    LF_ASSUME(len >= 0);

    switch (len) {
      case 0:
        co_return;
      case 1:
        std::invoke(fn, *head);
        co_return;
    }

    auto mid = head + (len / 2);
    auto sc = co_await scope();
    co_await sc.fork(for_each_overload{}, head, mid, fn);
    co_await sc.call(for_each_overload{}, mid, tail, fn);
    co_await sc.join();
  }

  // (3) range + n -> dispatches to (1) or (2)
  template <worker_context Context,
            std::ranges::random_access_range Range,
            std::indirectly_unary_invocable<std::ranges::iterator_t<Range>> Fn>
    requires std::ranges::sized_range<Range>
  static auto
  operator()(env<Context> /* env */, Range &&range, std::ranges::range_difference_t<Range> n, Fn fn)
      -> task<Context> {

    LF_ASSUME(n > 0);

    auto sc = co_await scope();
    if (n == 1) {
      co_await sc.call(
          for_each_overload{}, std::ranges::begin(range), std::ranges::end(range), std::move(fn));
    } else {
      co_await sc.call(
          for_each_overload{}, std::ranges::begin(range), std::ranges::end(range), n, std::move(fn));
    }
    co_await sc.join();
  }

  // (4) range, n == 1 -> dispatches to (2)
  template <worker_context Context,
            std::ranges::random_access_range Range,
            std::indirectly_unary_invocable<std::ranges::iterator_t<Range>> Fn>
    requires std::ranges::sized_range<Range>
  static auto operator()(env<Context> /* env */, Range &&range, Fn fn) -> task<Context> {
    auto sc = co_await scope();
    co_await sc.call(for_each_overload{}, std::ranges::begin(range), std::ranges::end(range), std::move(fn));
    co_await sc.join();
  }
};

export inline constexpr for_each_overload for_each = {};

} // namespace lf
