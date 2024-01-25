#ifndef BF260626_9180_496A_A893_A1A7F2B6781E
#define BF260626_9180_496A_A893_A1A7F2B6781E

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>   // for invocable
#include <functional> // for identity, invoke
#include <iterator>   // for iter_difference_t, random_access_iterator
#include <ranges>     // for begin, end, iterator_t, range_difference_t

#include "libfork/algorithm/constraints.hpp" // for indirectly_scannable, projected
#include "libfork/core/control_flow.hpp"     // for call, rethrow_if_exception, fork, join
#include "libfork/core/just.hpp"             // for just
#include "libfork/core/macro.hpp"            // for LF_STATIC_CALL, LF_STATIC_CONST, LF_CATCH_ALL
#include "libfork/core/task.hpp"             // for task

/**
 * @file scan.hpp
 *
 * @brief Implementation of a parallel scan.
 */

namespace lf {

namespace impl {

/**
 * @brief Reduction up-sweep of the scan.
 *
 * See https://en.wikipedia.org/wiki/Prefix_sum#Parallel_algorithms this computes the reduction stage of the
 * work-efficient algorithm. This is chunked and recursive. Essentially, we recursively divide the input into
 * smaller chunk (building an implicit balanced binary tree) until the chunk size is reached. Then we sweep
 * from the leaves of the tree (the chunks) to the root. At each tier we replace the right child with the sum
 * of the left and right child.
 *
 * Example:
 *
 * Input: [1, 1, 1, 1, 1, 1, 1]
 *
 * Build tree until n = 2:
 *
 * Partition 0: [1, 1, 1, 1, 1, 1, 1]
 *                     |- l_child  |- r_child
 * Partition 1: [1, 1, 1][1, 1, 1, 1]
 * Partition 2: [1][1, 1][1, 1][1, 1]
 *
 * At the leaves we compute the scan of each chunk:
 *
 * Partition 0: [1, 1, 2, 1, 2, 1, 2] <- views so they change as well
 * Partition 1: [1, 1, 2][1, 2, 1, 2] <- ^
 * Partition 2: [1][1, 2][1, 2][1, 2] <- Scan each chunk.
 *
 * Then we sweep up the tree:
 *
 * Partition 0: [1, 1, 3, 1, 2, 1, 4]
 * Partition 1: [1, 1, 3][1, 2, 1, 4] <- combining below into right child
 * Partition 2: [1][1, 2][1, 2][1, 2]
 *
 * Partition 0: [1, 1, 3, 1, 2, 1, 7] <- again combining below into right child
 * Partition 1: [1, 1, 3][1, 2, 1, 4]
 * Partition 2: [1][1, 2][1, 2][1, 2]
 *
 *
 * Final views at all partition levels:
 *
 * Partition 0: [1, 1, 3, 1, 2, 1, 7]
 * Partition 1: [1, 1, 3][1, 2, 1, 7]
 * Partition 2: [1][1, 3][1, 2][1, 7]
 */
template <bool InPlace>
inline constexpr auto reduction_sweep =
    []<std::random_access_iterator I,
       std::sized_sentinel_for<I> S,
       std::random_access_iterator O,
       class Proj,
       indirectly_scannable<O, projected<I, Proj>> Bop>(auto reduction_sweep, //
                                                        I beg,
                                                        S end,
                                                        std::iter_difference_t<I> n,
                                                        Bop bop,
                                                        Proj proj,
                                                        O out) LF_STATIC_CALL -> task<void> {
  //
  constexpr bool async_bop = !std::invocable<Bop &, std::iter_reference_t<O>, std::iter_reference_t<O>>;
  std::iter_difference_t<I> size = end - beg;
  //
  if (size <= n) {

    if constexpr (!(InPlace && std::is_same_v<std::identity, Proj>)) {
      *out = co_await just(proj)(*beg);
    }

    for (++beg; beg != end; ++beg) {

      auto prev = out;
      ++out;

      if constexpr (InPlace) {
        LF_ASSERT(out == beg.base());
      }

      if constexpr (async_bop) {
        co_await call(out, bop)(*prev, co_await just(proj)(*beg));
        co_await rethrow_if_exception;
      } else {
        *out = std::invoke(bop, *prev, co_await just(proj)(*beg));
      //  *out = *prev + *beg;  
      }
    }

    co_return;
  }
  // Recurse to smaller chunks.
  std::iter_difference_t<I> half = size / 2;
  I mid = beg + half;

  // clang-format off

  LF_TRY {
    co_await lf::fork(reduction_sweep)(beg, mid, n, bop, proj, out);
    co_await lf::call(reduction_sweep)(mid, end, n, bop, proj, out + half);
  } LF_CATCH_ALL {
    reduction_sweep.stash_exception();
  }

  // clang-format on

  co_await lf::join;

  // Accumulate in rhs of output chunk.
  // Require bop is a common_semigroup over output iter with with l and r value refs.
  O l_child = out + (half - 1);
  O r_child = out + (size - 1);

  if constexpr (async_bop) {
    co_await call(r_child, bop)(*l_child, std::ranges::iter_move(r_child));
    co_await rethrow_if_exception;
  } else {
    *r_child = bop(*l_child, std::ranges::iter_move(r_child));
  }
};

/**
 * @brief The down sweep of the scan.
 *
 * Here we recurse from the root to the leaves, at each stage if the sub-tree has a sibling to the left
 * we merge the reduction into the left child preserving the invariant that the left child has the prefix-sum
 *
 *After an up-sweep/reduction:

 * Partition 0: [1, 1, 3, 1, 2, 1, 7] <- no left sibling
 * Partition 1: [1, 1, 3][1, 2, 1, 7] <- right sub-tree has left sibling, update left child
 * Partition 2: [1][1, 3][1, 2][1, 7]
 *
 * After update 2 -> 2 + 3 (left sibling carries over):
 *
 * Partition 0: [1, 1, 3, 1, 5, 1, 7]
 * Partition 1: [1, 1, 3][1, 5, 1, 7]
 * Partition 2: [1][1, 3][1, 5][1, 7]
 *
 * Once we hit the chunk level update the first n - 1 elements of each chunk with the carried value.
 */
inline constexpr auto rhs_down_sweep =
    []<std::random_access_iterator O, std::sized_sentinel_for<O> S, typename Bop>(auto rhs_down_sweep, //
                                                                                  O beg,
                                                                                  S end,
                                                                                  std::iter_difference_t<O> n,
                                                                                  Bop bop)
        LF_STATIC_CALL -> task<void> {
  /**
   * Chunks looks like:
   *
   *  [a, b, c, acc_prev] [d, e, f, acc] [h, i, j, acc_next]
   *                       ^- beg         ^- end
   */
  constexpr bool async_bop = !std::invocable<Bop &, std::iter_reference_t<O>, std::iter_reference_t<O>>;
  std::iter_difference_t<O> size = end - beg;
  O acc_prev = beg - 1; // Carried/previous accumulation

  if (size <= n) {
    for (; beg != end - 1; ++beg) {
      if constexpr (async_bop) {
        co_await call(beg, bop)(*acc_prev, std::ranges::iter_move(beg));
        co_await rethrow_if_exception;
      } else {
        *beg = bop(*acc_prev, std::ranges::iter_move(beg));
      }
    }
    co_return;
  }

  std::iter_difference_t<O> half = size / 2; //
  O rhs = beg + (half - 1);                  // This is the left child of the tree.

  if constexpr (async_bop) {
    co_await call(rhs, bop)(*acc_prev, std::ranges::iter_move(rhs));
    co_await rethrow_if_exception;
  } else {
    *rhs = bop(*acc_prev, std::ranges::iter_move(rhs));
  }

  O mid = beg + half;

  // clang-format off

  LF_TRY {
    co_await lf::fork(rhs_down_sweep)(beg, mid, n, bop);
    co_await lf::call(rhs_down_sweep)(mid, end, n, bop);
  } LF_CATCH_ALL {
    rhs_down_sweep.stash_exception();
  }

  // clang-format on

  co_await lf::join;
};

/**
 * @brief Down-sweep of sub-tree with no left sibling.
 */
inline constexpr auto lhs_down_sweep =
    []<std::random_access_iterator O, std::sized_sentinel_for<O> S, typename Bop>(auto lhs_down_sweep, //
                                                                                  O beg,
                                                                                  S end,
                                                                                  std::iter_difference_t<O> n,
                                                                                  Bop bop)
        LF_STATIC_CALL -> task<void> {
  if (auto size = end - beg; size > n) {

    auto mid = beg + size / 2;

    // clang-format off
     
        LF_TRY {
          co_await lf::fork(lhs_down_sweep)(beg, mid, n, bop);
          co_await lf::call(rhs_down_sweep)(mid, end, n, bop);
        } LF_CATCH_ALL {
          lhs_down_sweep.stash_exception();
        }

    // clang-format on

    co_await lf::join;
  }
};

/**
 * @brief Eight overloads of scan for (iterator/range, chunk/in_place, n = 1/n != 1).
 */
struct scan_overload {
  /**
   * @brief [iterator,chunk,output] version (5-6)
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::random_access_iterator O,
            class Proj = std::identity,
            indirectly_scannable<O, projected<I, Proj>> Bop>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 O out,
                                 std::iter_difference_t<I> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (std::iter_difference_t<I> size = end - beg; size > 0) {
      co_await lf::just(reduction_sweep<false>)(beg, end, n, bop, proj, out);
      co_await lf::just(lhs_down_sweep)(out, out + size, n, bop);
    }
  }
  /**
   * @brief [iterator,n = 1,output] version (4-5)
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            std::random_access_iterator O,
            class Proj = std::identity,
            indirectly_scannable<O, projected<I, Proj>> Bop>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 O out,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (std::iter_difference_t<I> size = end - beg; size > 0) {
      co_await lf::just(reduction_sweep<false>)(beg, end, std::iter_difference_t<I>(1), bop, proj, out);
      co_await lf::just(lhs_down_sweep)(out, out + size, std::iter_difference_t<I>(1), bop);
    }
  }
  /**
   * @brief [iterator,chunk,in_place] version (4-5)
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_scannable<I, projected<std::move_iterator<I>, Proj>> Bop>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 std::iter_difference_t<I> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (std::iter_difference_t<I> size = end - beg; size > 0) {
      co_await lf::just(reduction_sweep<true>)(std::move_iterator(beg),
                                               std::move_sentinel(end),
                                               n,
                                               bop,
                                               proj,
                                               beg //
      );
      co_await lf::just(lhs_down_sweep)(beg, end, n, bop);
    }
    co_return;
  }
  /**
   * @brief [iterator,n = 1,in_place] version.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_scannable<I, projected<std::move_iterator<I>, Proj>> Bop>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (std::iter_difference_t<I> size = end - beg; size > 0) {
      co_await lf::just(reduction_sweep<true>)(std::move_iterator(beg),
                                               std::move_sentinel(end),
                                               std::iter_difference_t<I>(1),
                                               bop,
                                               proj,
                                               beg //
      );
      co_await lf::just(lhs_down_sweep)(beg, end, std::iter_difference_t<I>(1), bop);
    }
  }
  /**
   * @brief [range,chunk,output] version (5-6)
   */
  template <std::ranges::random_access_range R,
            std::random_access_iterator O,
            class Proj = std::identity,
            indirectly_scannable<O, projected<std::ranges::iterator_t<R>, Proj>> Bop>
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 O out,
                                 std::ranges::range_difference_t<R> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (!std::ranges::empty(range)) {
      co_await lf::just(reduction_sweep<false>)(
          std::ranges::begin(range), std::ranges::end(range), n, bop, proj, out //
      );
      co_await lf::just(lhs_down_sweep)(out, out + std::ranges::ssize(range), n, bop);
    }
  }
  /**
   * @brief [range,n = 1,output] version (4-5)
   */
  template <std::ranges::random_access_range R,
            std::random_access_iterator O,
            class Proj = std::identity,
            indirectly_scannable<O, projected<std::ranges::iterator_t<R>, Proj>> Bop>
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 O out,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (!std::ranges::empty(range)) {
      co_await lf::just(reduction_sweep<false>)(std::ranges::begin(range),
                                                std::ranges::end(range),
                                                std::ranges::range_difference_t<R>(1),
                                                bop,
                                                proj,
                                                out //
      );
      co_await lf::just(lhs_down_sweep)(
          out, out + std::ranges::ssize(range), std::ranges::range_difference_t<R>(1), bop //
      );
    }
  }
  /**
   * @brief [range,chunk,in_place] version (4-5)
   */
  template <std::ranges::random_access_range R,
            class Proj = std::identity,
            indirectly_scannable<std::ranges::iterator_t<R>,
                                 projected<std::move_iterator<std::ranges::iterator_t<R>>, Proj>> Bop>
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 std::ranges::range_difference_t<R> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (!std::ranges::empty(range)) {
      co_await lf::just(reduction_sweep<true>)(std::move_iterator(std::ranges::begin(range)),
                                               std::move_sentinel(std::ranges::end(range)),
                                               n,
                                               bop,
                                               proj,
                                               std::ranges::begin(range) //
      );
      co_await lf::just(lhs_down_sweep)(std::ranges::begin(range), std::ranges::end(range), n, bop);
    }
  }
  /**
   * @brief [range,n = 1,in_place] version.
   */
  template <std::ranges::random_access_range R,
            class Proj = std::identity,
            indirectly_scannable<std::ranges::iterator_t<R>,
                                 projected<std::move_iterator<std::ranges::iterator_t<R>>, Proj>> Bop>
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    if (!std::ranges::empty(range)) {
      co_await lf::just(reduction_sweep<true>)(std::move_iterator(std::ranges::begin(range)),
                                               std::move_sentinel(std::ranges::end(range)),
                                               std::ranges::range_difference_t<R>(1),
                                               bop,
                                               proj,
                                               std::ranges::begin(range) //
      );
      co_await lf::just(lhs_down_sweep)(
          std::ranges::begin(range), std::ranges::end(range), std::ranges::range_difference_t<R>(1), bop //
      );
    }
  }
};

} // namespace impl

/**
 * @brief A parallel implementation of `std::inclusive_scan` that accepts generalized ranges and projections.
 *
 * \rst
 *
 * Effective call signature:
 *
 * .. code ::
 *
 *    template <std::random_access_iterator I,
 *              std::sized_sentinel_for<I> S,
 *              std::random_access_iterator O,
 *              class Proj = std::identity,
 *              indirectly_scannable<O, projected<I, Proj>> Bop
 *              >
 *    void scan(I beg, S end, O out, std::iter_difference_t<I> n, Bop bop, Proj proj = {});
 *
 * Overloads exist for a random-access range (instead of ``head`` and ``tail``), in place scans (omit the
 * `out` iterator) and, the chunk size, ``n``, can be omitted (which will set ``n = 1``).
 *
 * Exemplary usage:
 *
 * .. code::
 *
 *    co_await just[scan](in, out.begin(), std::plus<>{});
 *
 * \endrst
 *
 * This computes the cumulative sum of the input and stores it in the output-range e.g. `[1, 2, 2, 1] -> [1,
 * 3, 5, 6]`.
 *
 * The input and output ranges must either be distinct (i.e. non-overlapping) or the same range.
 *
 * If the binary operator or projection handed to `scan` are async functions, then they will be
 * invoked asynchronously, this allows you to launch further tasks recursively.
 *
 * Unlike the `std::` variations, this function will make an implementation defined number of
 * copies of the function objects and may invoke these copies concurrently.
 */
inline constexpr impl::scan_overload scan = {};

} // namespace lf

#endif /* BF260626_9180_496A_A893_A1A7F2B6781E */
