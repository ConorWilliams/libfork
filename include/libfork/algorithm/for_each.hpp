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
#include <type_traits>

#include "libfork/core.hpp"

#include "libfork/algorithm/concepts.hpp"

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
            typename Proj = std::identity,
            indirectly_unary_invocable<std::projected<I, Proj>> Fun>
  LF_STATIC_CALL auto
  operator()(auto for_each, I head, S tail, std::iter_difference_t<I> n, Fun fun, Proj proj = {})
      LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(n > 0);

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len >= 0);

    if (len == 0) [[unlikely]] {
      co_return;
    }

    if (len <= n) {
      for (; head != tail; ++head) {
        if constexpr (async_fn<Fun>) {
          co_await lf::call(fun)(std::invoke(proj, *head));
        } else {
          std::invoke(fun, std::invoke(proj, *head));
        }
      }
      co_return;
    }

    auto mid = head + (len / 2);

    co_await lf::fork(for_each)(head, mid, n, fun, proj);
    co_await lf::call(for_each)(mid, tail, n, fun, proj);

    co_await lf::join;
  }

  /**
   * @brief Divide and conquer n = 1 version.
   *
   * This is an efficient implementation for sized random access ranges.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Proj = std::identity,
            indirectly_unary_invocable<std::projected<I, Proj>> Fun>
  LF_STATIC_CALL auto
  operator()(auto for_each, I head, S tail, Fun fun, Proj proj = {}) LF_STATIC_CONST->lf::task<> {

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len >= 0);

    if (len == 0) [[unlikely]] {
      co_return;
    }

    if (len <= 1) {
      if constexpr (async_fn<Fun>) {
        co_await lf::call(fun)(std::invoke(proj, *head));
      } else {
        std::invoke(fun, std::invoke(proj, *head));
      }
      co_return;
    }

    auto mid = head + (len / 2);

    co_await lf::fork(for_each)(head, mid, fun, proj);
    co_await lf::call(for_each)(mid, tail, fun, proj);

    co_await lf::join;
  }

  /**
   * @brief Range version, dispatches to the iterator version.
   *
   * This will dispatch to `n = 1` specialization if `n = 1`
   */
  template <std::ranges::random_access_range Range,
            typename Proj = std::identity,
            indirectly_unary_invocable<std::projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto operator()(auto for_each,
                                 Range &&range,
                                 std::ranges::range_difference_t<Range> n,
                                 Fun fun,
                                 Proj proj = {}) LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(n > 0);

    if (n == 1) {
      co_await for_each(std::ranges::begin(range), std::ranges::end(range), fun, proj);
    } else {
      co_await for_each(std::ranges::begin(range), std::ranges::end(range), n, fun, proj);
    }
  };

  /**
   * @brief Range n = 1, version, dispatches to the iterator version.
   */
  template <std::ranges::random_access_range Range,
            typename Proj = std::identity,
            indirectly_unary_invocable<std::projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto
  operator()(auto for_each, Range &&range, Fun fun, Proj proj = {}) LF_STATIC_CONST->lf::task<> {
    co_await for_each(std::ranges::begin(range), std::ranges::end(range), fun, proj);
  };
};

} // namespace impl

/**
 * @brief A parallel implementation of `std::ranges::for_each`.
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
 * This will set each element of `v` to `0` in parallel using a n size of ``10``. The n size is the
 * number of elements each task will process, the n size can be omitted and defaults to ``1``.
 *
 * If the function handed to `for_each` is an ``async`` function, then the function will be called
 * asynchronously, this allows you to launch further tasks recursively. The projection is required
 * to be a regular function.
 *
 * Unlike `std::ranges::for_each`, this function will make an implementation defined number of copies
 * of the function objects and may invoke these copies concurrently. Hence, it is assumed function
 * objects are cheap to copy.
 */
inline constexpr async for_each = impl::for_each_overload{};

} // namespace lf

#endif /* C5165911_AD64_4DAC_ACEB_DDB9B718B3ED */
