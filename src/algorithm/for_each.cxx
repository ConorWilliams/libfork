module;
#include "libfork/__impl/assume.hpp"
export module libfork.algorithm:for_each;

import std;

import libfork.core;

namespace lf {

namespace impl {

struct for_each_overload {

  // (1) iterator-pair, chunk size n > 1
  template <typename Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>
  static auto operator()(env<Context>,
                         I head,
                         S tail,
                         std::iter_difference_t<I> n,
                         Fun fun,
                         Proj proj = {}) -> task<void, Context> {
    LF_ENSURE(n > 0);
    auto len = tail - head;
    LF_ENSURE(len >= 0);

    if (len == 0) {
      co_return;
    }

    if (len <= n) {
      for (; head != tail; ++head) {
        std::invoke(fun, std::invoke(proj, *head));
      }
      co_return;
    }

    auto mid = head + (len / 2);
    auto sc = co_await scope();
    co_await sc.fork(for_each_overload{}, head, mid, n, fun, proj);
    co_await sc.call(for_each_overload{}, mid, tail, n, fun, proj);
    co_await sc.join();
  }

  // (2) iterator-pair, n == 1 specialization (no n parameter)
  template <typename Context,
            std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>
  static auto
  operator()(env<Context>, I head, S tail, Fun fun, Proj proj = {}) -> task<void, Context> {
    auto len = tail - head;
    LF_ENSURE(len >= 0);

    switch (len) {
      case 0:
        co_return;
      case 1:
        std::invoke(fun, std::invoke(proj, *head));
        co_return;
      default: {
        auto mid = head + (len / 2);
        auto sc = co_await scope();
        co_await sc.fork(for_each_overload{}, head, mid, fun, proj);
        co_await sc.call(for_each_overload{}, mid, tail, fun, proj);
        co_await sc.join();
      }
    }
  }

  // (3) range + n -> dispatches to (1) or (2)
  template <typename Context,
            std::ranges::random_access_range Range,
            typename Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range>
  static auto operator()(env<Context>,
                         Range &&range,
                         std::ranges::range_difference_t<Range> n,
                         Fun fun,
                         Proj proj = {}) -> task<void, Context> {
    LF_ENSURE(n > 0);
    auto sc = co_await scope();
    if (n == 1) {
      co_await sc.call(for_each_overload{},
                       std::ranges::begin(range),
                       std::ranges::end(range),
                       std::move(fun),
                       std::move(proj));
    } else {
      co_await sc.call(for_each_overload{},
                       std::ranges::begin(range),
                       std::ranges::end(range),
                       n,
                       std::move(fun),
                       std::move(proj));
    }
    co_await sc.join();
  }

  // (4) range, n == 1 -> dispatches to (2)
  template <typename Context,
            std::ranges::random_access_range Range,
            typename Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range>
  static auto
  operator()(env<Context>, Range &&range, Fun fun, Proj proj = {}) -> task<void, Context> {
    auto sc = co_await scope();
    co_await sc.call(for_each_overload{},
                     std::ranges::begin(range),
                     std::ranges::end(range),
                     std::move(fun),
                     std::move(proj));
    co_await sc.join();
  }
};

} // namespace impl

export inline constexpr impl::for_each_overload for_each = {};

} // namespace lf
