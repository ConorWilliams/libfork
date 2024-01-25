#ifndef DFB97DB6_8A5B_401E_AB7B_A386D71F4EE1
#define DFB97DB6_8A5B_401E_AB7B_A386D71F4EE1

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <functional> // for identity
#include <iterator>   // for random_access_iterator, indirectly_copyable
#include <ranges>     // for iterator_t, begin, end, random_access_range

#include "libfork/algorithm/constraints.hpp" // for projected, indirectly_unary_invocable
#include "libfork/core/control_flow.hpp"     // for call, fork, join
#include "libfork/core/just.hpp"             // for just
#include "libfork/core/macro.hpp"            // for LF_ASSERT, LF_STATIC_CALL, LF_STATIC_CONST
#include "libfork/core/task.hpp"             // for task

/**
 * @file map.hpp
 *
 * @brief A parallel implementation of `std::map`.
 */

namespace lf {

namespace impl {

/**
 * @brief Overload set for `lf::map`.
 */
struct map_overload {
  /**
   * @brief Divide and conquer implementation.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::random_access_iterator O,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<I, Proj>> Fun>
    requires std::indirectly_copyable<projected<I, Proj, Fun>, O>
  LF_STATIC_CALL auto
  operator()(auto map, I head, S tail, O out, std::iter_difference_t<I> n, Fun fun, Proj proj = {})
      LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(n > 0);

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len >= 0);

    if (len == 0) {
      co_return;
    }

    if (len <= n) {
      for (; head != tail; ++head, ++out) {
        *out = co_await lf::just(fun)(co_await just(proj)(*head));
      }
      co_return;
    }

    auto dif = (len / 2);
    auto mid = head + dif;

    // clang-format off

    LF_TRY {
      co_await lf::fork(map)(head, mid, out, n, fun, proj);
      co_await lf::call(map)(mid, tail, out + dif, n, fun, proj);
    } LF_CATCH_ALL { 
      map.stash_exception(); 
    }

    // clang-format on

    co_await lf::join;
  }

  /**
   * @brief Divide and conquer n = 1 version.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::random_access_iterator O,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<I, Proj>> Fun>
    requires std::indirectly_copyable<projected<I, Proj, Fun>, O>
  LF_STATIC_CALL auto
  operator()(auto map, I head, S tail, O out, Fun fun, Proj proj = {}) LF_STATIC_CONST->lf::task<> {

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len >= 0);

    switch (len) {
      case 0:
        break;
      case 1:
        *out = co_await lf::just(fun)(co_await just(proj)(*head));
        break;
      default:
        auto dif = (len / 2);
        auto mid = head + dif;

        // clang-format off

        LF_TRY {  
          co_await lf::fork(map)(head, mid, out, fun, proj);
          co_await lf::call(map)(mid, tail, out + dif, fun, proj);
        } LF_CATCH_ALL { 
          map.stash_exception(); 
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
            std::random_access_iterator O,
            typename Proj = std::identity,
            indirectly_unary_invocable<projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range> &&
             std::indirectly_copyable<projected<std::ranges::iterator_t<Range>, Proj, Fun>, O>
             LF_STATIC_CALL auto operator()(auto map,
                                            Range &&range,
                                            O out,
                                            std::ranges::range_difference_t<Range> n,
                                            Fun fun,
                                            Proj proj = {}) LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(n > 0);

    if (n == 1) {
      co_await just(map)(std::ranges::begin(range), std::ranges::end(range), out, fun, proj);
    } else {
      co_await just(map)(std::ranges::begin(range), std::ranges::end(range), out, n, fun, proj);
    }
  }

  /**
   * @brief Range n = 1, version, dispatches to the iterator version.
   */
  template <std::ranges::random_access_range Range,
            typename Proj = std::identity,
            std::random_access_iterator O,
            indirectly_unary_invocable<projected<std::ranges::iterator_t<Range>, Proj>> Fun>
    requires std::ranges::sized_range<Range> &&
             std::indirectly_copyable<projected<std::ranges::iterator_t<Range>, Proj, Fun>, O>
             LF_STATIC_CALL auto
             operator()(auto map, Range &&range, O out, Fun fun, Proj proj = {}) LF_STATIC_CONST->lf::task<> {
    co_await lf::just(map)(
        std::ranges::begin(range), std::ranges::end(range), out, std::move(fun), std::move(proj) //
    );
  }
};

} // namespace impl

/**
 * @brief A parallel variation of `std::transform`.
 *
 * \rst
 *
 * Effective call signature:
 *
 * .. code ::
 *
 *    template <std::random_access_iterator I,
 *              std::sized_sentinel_for<I> S,
 *              std::random_access_iterator O
 *              typename Proj = std::identity,
 *              indirectly_unary_invocable<projected<I, Proj>> Fun
 *              >
 *      requires std::indirectly_copyable<projected<I, Proj, Fun>, O>
 *    auto map(I head, S tail, O out, std::iter_difference_t<I> n, Fun fun, Proj proj = {}) -> lf::task<>;
 *
 * Overloads exist for a random access range (instead of ``head`` and ``tail``) and ``n`` can be omitted
 * (which will set ``n = 1``).
 *
 * Exemplary usage:
 *
 * .. code::
 *
 *    std::vector<int> out(v.size());
 *
 *    co_await just[map](v, out.begin(), 10, [](int const& elem) {
 *      return elem + 1;
 *    });
 *
 * \endrst
 *
 * This will set each element of `out` to one more than corresponding element in `v` using
 * a chunk size of ``10``.
 *
 * If the function or projection handed to `map` are async functions, then they will be
 * invoked asynchronously, this allows you to launch further tasks recursively.
 *
 * Unlike `std::transform`, this function will make an implementation defined number of copies
 * of the function objects and may invoke these copies concurrently. Hence, it is assumed function
 * objects are cheap to copy.
 */
inline constexpr impl::map_overload map = {};

} // namespace lf

#endif /* DFB97DB6_8A5B_401E_AB7B_A386D71F4EE1 */
