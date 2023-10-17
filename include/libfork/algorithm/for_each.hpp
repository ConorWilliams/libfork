#ifndef C5165911_AD64_4DAC_ACEB_DDB9B718B3ED
#define C5165911_AD64_4DAC_ACEB_DDB9B718B3ED

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>

#include "libfork/core.hpp"

/**
 * @file for_each.hpp
 *
 * @brief A parallel implementation of `std::for_each`.
 */

namespace lf {

namespace impl {

struct for_each_overload {

  /**
   * @brief Divide and conquer implementation.
   *
   * This is an efficient implementation for sized random access ranges.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fun>
  LF_STATIC_CALL auto
  operator()(auto, I head, S tail, std::iter_difference_t<I> grain, Fun fun) LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(grain > 0);

    std::iter_difference_t<I> len = tail - head;

    if (len <= grain) {
      for (; head != tail; ++head) {
        if constexpr (async_fn<Fun>) {
          co_await lf::call(fun)(*head);
        } else {
          std::invoke(fun, *head);
        }
      }
      co_return;
    }

    constexpr async for_each = for_each_overload{};

    auto mid = head + (len / 2);

    co_await lf::fork(for_each)(head, mid, grain, fun);
    co_await lf::call(for_each)(mid, tail, grain, fun);

    co_await lf::join;
  }

  /**
   * @brief Divide and conquer grain=1 version.
   *
   * This is an efficient implementation for sized random access ranges.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::indirectly_unary_invocable<I> Fun>
  LF_STATIC_CALL auto operator()(auto, I head, S tail, Fun fun) LF_STATIC_CONST->lf::task<> {

    std::iter_difference_t<I> len = tail - head;

    if (len <= 1) {
      if constexpr (async_fn<Fun>) {
        co_await lf::call(fun)(*head);
      } else {
        std::invoke(fun, *head);
      }
      co_return;
    }

    constexpr async for_each = for_each_overload{};

    auto mid = head + (len / 2);

    co_await lf::fork(for_each)(head, mid, fun);
    co_await lf::call(for_each)(mid, tail, fun);

    co_await lf::join;
  }

  /**
   * @brief This is a less-efficient implementation for forward iterators.
   */
  template <std::forward_iterator I, std::sentinel_for<I> S, std::indirectly_unary_invocable<I> Fun>
  LF_STATIC_CALL auto
  operator()(auto, I head, S tail, std::iter_difference_t<I> grain, Fun fun) LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(grain > 0);

    constexpr async for_each = [](auto, I begin, I end, Fun fun) LF_STATIC_CALL -> lf::task<> {
      for (; begin != end; ++begin) {
        if constexpr (async_fn<Fun>) {
          co_await lf::call(fun)(*begin);
        } else {
          std::invoke(fun, *begin);
        }
      }
    };

    while (head != tail) {
      I end = std::ranges::next(head, grain, tail);
      co_await lf::fork(for_each)(head, end, fun);
      head = end;
    }

    co_await lf::join;
  }

  /**
   * @brief Grain=1 version for forward iterators.
   *
   * This is a less-efficient implementation for forward iterators.
   */
  template <std::forward_iterator I, std::sentinel_for<I> S, std::indirectly_unary_invocable<I> Fun>
  LF_STATIC_CALL auto operator()(auto, I head, S tail, Fun fun) LF_STATIC_CONST->lf::task<> {

    constexpr async invoke_fn = [](auto, I iter, Fun fun) LF_STATIC_CALL -> lf::task<> {
      std::invoke(fun, *iter);
      co_return;
    };

    for (; head != tail; ++head) {
      if constexpr (async_fn<Fun>) {
        co_await lf::fork(fun)(*head);
      } else {
        co_await lf::fork(invoke_fn)(head, fun);
      }
    }

    co_await lf::join;
  }

  /**
   * @brief Range version, dispatches to the iterator version.
   */
  template <std::ranges::forward_range Range,
            std::indirectly_unary_invocable<std::ranges::iterator_t<Range>> Fun>
  LF_STATIC_CALL auto operator()(auto, Range &&range, std::ranges::range_difference_t<Range> grain, Fun fun)
      LF_STATIC_CONST->lf::task<> {
    co_await lf::async<for_each_overload>{}(std::ranges::begin(range), std::ranges::end(range), grain, fun);
  };

  /**
   * @brief Range grain=1, version, dispatches to the iterator version.
   */
  template <std::ranges::forward_range Range,
            std::indirectly_unary_invocable<std::ranges::iterator_t<Range>> Fun>
  LF_STATIC_CALL auto operator()(auto, Range &&range, Fun fun) LF_STATIC_CONST->lf::task<> {
    co_await lf::async<for_each_overload>{}(std::ranges::begin(range), std::ranges::end(range), fun);
  };
};

} // namespace impl

/**
 * @brief A parallel implementation of `std::for_each`.
 *
 * \rst
 *
 * Exemplary usage:
 *
 * .. code::
 *
 *    co_await lf::for_each(v, 10, [](auto &elem) {
 *      elem = 0;
 *    });
 *
 * \endrst
 *
 * This will set each element of `v` to `0` in parallel using a grain size of ``10``. The grain size is the
 * number of elements each task will process, the grain size can be omitted and defaults to ``1``.
 *
 * If the function handed to `for_each` is an ``async`` function, then the function will be called
 * asynchronously, this allows you to launch further tasks recursively.
 *
 * This function has overloads for `forward_ranges` and sized `random_access_range` with the latter being more
 * efficient.
 *
 */
inline constexpr async for_each = impl::for_each_overload{};

} // namespace lf

#endif /* C5165911_AD64_4DAC_ACEB_DDB9B718B3ED */
