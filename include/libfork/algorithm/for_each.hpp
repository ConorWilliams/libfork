#ifndef C5165911_AD64_4DAC_ACEB_DDB9B718B3ED
#define C5165911_AD64_4DAC_ACEB_DDB9B718B3ED

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <functional> // for identity
#include <iterator>   // for iter_difference_t, random_access_iterator
#include <ranges>     // for begin, end, iterator_t, random_access_range

#include "libfork/algorithm/constraints.hpp" // for indirectly_unary_invocable, projected
#include "libfork/core/control_flow.hpp"     // for call, fork, join
#include "libfork/core/just.hpp"             // for just
#include "libfork/core/macro.hpp"            // for LF_ASSERT, LF_STATIC_CALL, LF_STATIC_CONST
#include "libfork/core/task.hpp"             // for task

/**
 * @file for_each.hpp
 *
 * @brief A parallel implementation of `std::for_each`.
 */

namespace lf {

namespace impl {

/**
 * @brief Overload set for `lf::for_each`.
 */
struct for_each_overload {

  /**
   * @brief Divide and conquer implementation.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<I, Proj>> Fun>
  LF_STATIC_CALL auto
  operator()(auto for_each, I head, S tail, std::iter_difference_t<I> n, Fun fun, Proj proj = {})
      LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(n > 0);

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len >= 0);

    if (len == 0) {
      co_return;
    }

    if (len <= n) {
      for (; head != tail; ++head) {
        co_await lf::just(fun)(co_await just(proj)(*head));
      }
      co_return;
    }

    auto mid = head + (len / 2);

    // clang-format off

    LF_TRY {
      co_await lf::fork(for_each)(head, mid, n, fun, proj);
      co_await lf::call(for_each)(mid, tail, n, fun, proj);
    } LF_CATCH_ALL { 
      for_each.stash_exception(); 
    }

    // clang-format on

    co_await lf::join;
  }

  /**
   * @brief Divide and conquer n = 1 version.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<I, Proj>> Fun>
  LF_STATIC_CALL auto
  operator()(auto for_each, I head, S tail, Fun fun, Proj proj = {}) LF_STATIC_CONST->lf::task<> {

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len >= 0);

    switch (len) {
      case 0:
        break;
      case 1:
        co_await lf::just(fun)(co_await just(proj)(*head));
        break;
      default:
        auto mid = head + (len / 2);

        // clang-format off

        LF_TRY {
          co_await lf::fork(for_each)(head, mid, fun, proj);
          co_await lf::call(for_each)(mid, tail, fun, proj);
        } LF_CATCH_ALL { 
          for_each.stash_exception(); 
        }

        // clang-format on

        co_await lf::join;
    }
  }

  /**
   * @brief Range version, dispatches to the iterator version.
   *
   * This will dispatch to `n = 1` specialization if `n = 1`
   */
  template <std::ranges::random_access_range Range,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto
  operator()(auto for_each, Range &&range, std::ranges::range_difference_t<Range> n, Fun fun, Proj proj = {})
      LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(n > 0);

    if (n == 1) {
      co_await just(for_each)(std::ranges::begin(range), std::ranges::end(range), fun, proj);
    } else {
      co_await just(for_each)(std::ranges::begin(range), std::ranges::end(range), n, fun, proj);
    }
  }

  /**
   * @brief Range n = 1, version, dispatches to the iterator version.
   */
  template <std::ranges::random_access_range Range,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto
  operator()(auto for_each, Range &&range, Fun fun, Proj proj = {}) LF_STATIC_CONST->lf::task<> {
    co_await lf::just(for_each)(
        std::ranges::begin(range), std::ranges::end(range), std::move(fun), std::move(proj) //
    );
  }
};

} // namespace impl

/**
 * @brief A parallel implementation of `std::ranges::for_each`.
 *
 * \rst
 *
 * Effective call signature:
 *
 * .. code ::
 *
 *    template <std::random_access_iterator I,
 *              std::sized_sentinel_for<I> S,
 *              typename Proj = std::identity,
 *              indirectly_unary_invocable<projected<I, Proj>> Fun
 *              >
 *    auto for_each(I head, S tail, std::iter_difference_t<I> n, Fun fun, Proj proj = {}) -> lf::task<>;
 *
 * Overloads exist for a random access range (instead of ``head`` and ``tail``) and ``n`` can be omitted
 * (which will set ``n = 1``).
 *
 * Exemplary usage:
 *
 * .. code::
 *
 *    co_await just[for_each](v, 10, [](auto &elem) {
 *      elem = 0;
 *    });
 *
 * \endrst
 *
 * This will set each element of `v` to `0` in parallel using a chunk size of ``10``.
 *
 * If the function or projection handed to `for_each` are async functions, then they will be
 * invoked asynchronously, this allows you to launch further tasks recursively.
 *
 * Unlike `std::ranges::for_each`, this function will make an implementation defined number of copies
 * of the function objects and may invoke these copies concurrently. Hence, it is assumed function
 * objects are cheap to copy.
 */
inline constexpr impl::for_each_overload for_each = {};

} // namespace lf

#endif /* C5165911_AD64_4DAC_ACEB_DDB9B718B3ED */
