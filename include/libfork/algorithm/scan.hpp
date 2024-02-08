#ifndef BF260626_9180_496A_A893_A1A7F2B6781E
#define BF260626_9180_496A_A893_A1A7F2B6781E

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for same_as
#include <functional>  // for identity, invoke
#include <iterator>    // for random_access_iterator, sized_sentinel_for
#include <ranges>      // for begin, end, iterator_t, random_access_range
#include <type_traits> // for conditional_t

#include "libfork/algorithm/constraints.hpp" // for indirectly_scannable, projected
#include "libfork/core/control_flow.hpp"     // for call, dispatch, fork, join
#include "libfork/core/invocable.hpp"        // for async_invocable
#include "libfork/core/just.hpp"             // for just
#include "libfork/core/macro.hpp"            // for LF_STATIC_CALL, LF_STATIC_CONST, LF_PRAGMA_...
#include "libfork/core/tag.hpp"              // for tag, eager_throw_outside, sync_outside
#include "libfork/core/task.hpp"             // for task

/**
 * @file scan.hpp
 *
 * @brief Implementation of a parallel scan.
 */

namespace lf {

namespace impl {

/**
 * Operation propagates as:
 *
 * scan -> (scan, x)
 *    x can be scan if the left child completes synchronously, otherwise it must be a fold.
 *
 * fold -> (fold, fold)
 */
enum class op {
  scan,
  fold,
};

/**
 * Possible intervals:
 *
 * Full range: (lhs, rhs)
 * Sub range: (lhs, mid), (mid, mid), (mid, rhs)
 *
 *
 * Transformations:
 *
 * (lhs, rhs) -> (lhs, mid), (mid, rhs)
 *
 * (lhs, mid) -> (lhs, mid), (mid, mid)
 * (mid, rhs) -> (mid, mid), (mid, rhs)
 *
 * (mid, mid) -> (mid, mid), (mid, mid)
 *
 */
enum class interval {
  all,
  lhs,
  mid,
  rhs,
};

/**
 * @brief Get the interval to the left of the current interval.
 */
consteval auto l_child_of(interval ival) -> interval {
  switch (ival) {
    case interval::all:
    case interval::lhs:
      return interval::lhs;
    case interval::mid:
    case interval::rhs:
      return interval::mid;
    default:
      unreachable();
  }
}

/**
 * @brief Get the interval to the right of the current interval.
 */
consteval auto r_child_of(interval ival) -> interval {
  switch (ival) {
    case interval::all:
    case interval::rhs:
      return interval::rhs;
    case interval::lhs:
    case interval::mid:
      return interval::mid;
    default:
      unreachable();
  }
}

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
 *
 * As an optimization over the basic algorithm we scan the chunks on the left of the tree.
 */
template <std::random_access_iterator I, //
          std::sized_sentinel_for<I> S,  //
          class Proj,                    //
          class Bop,                     //
          std::random_access_iterator O, //
          interval Ival = interval::all, //
          op Op = op::scan               //
          >
struct rise_sweep {
  /**
   * @brief The iterator difference type of I.
   */
  using int_t = std::iter_difference_t<I>;
  /**
   * @brief The accumulator type of the reduction.
   */
  using acc_t = std::iter_value_t<O>;
  /**
   * @brief The type of the task returned by the algorithm.
   */
  using task_t = lf::task<std::conditional_t<Op == op::scan, I, void>>;
  /**
   * @brief Propagate the operation to the left child.
   */
  using up_lhs = rise_sweep<I, S, Proj, Bop, O, l_child_of(Ival), Op>;
  /**
   * @brief The right child's operation depends on the readiness of the left child.
   */
  template <op OpChild>
  using up_rhs = rise_sweep<I, S, Proj, Bop, O, r_child_of(Ival), OpChild>;
  /**
   * @brief If the binary operator is asynchronous, some optimizations can be done if it's not async.
   */
  static constexpr bool async_bop = async_invocable<Bop &, acc_t, acc_t>;
  /**
   * @brief Returns one-past-the-end of the scanned range.
   */
  LF_STATIC_CALL auto
  operator()(auto self, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->task_t {
    //
    int_t const size = end - beg;

    LF_ASSERT(size >= 1);

    static constexpr auto eager_call_outside = dispatch<tag::call, modifier::eager_throw_outside>;

    if (size <= n) {
      if constexpr (Op == op::fold) { // Equivalent to a fold over acc_t, left sibling not ready.

        static_assert(Ival != interval::lhs && Ival != interval::all, "left can always scan");

        if constexpr (Ival == interval::mid) {
          // Mid segment has a right sibling so do the fold.
          acc_t acc = acc_t(co_await lf::just(proj)(*beg));
          // The optimizer sometimes trips-up so we force a bit of unrolling.
          LF_PRAGMA_UNROLL(8)
          for (++beg; beg != end; ++beg) {
            if constexpr (async_bop) {
              co_await eager_call_outside(&acc, bop)(std::move(acc), co_await lf::just(proj)(*beg));
            } else {
              acc = std::invoke(bop, std::move(acc), co_await lf::just(proj)(*beg));
            }
          }
          // Store in the correct location in the output (in-case the scan is in-place).
          *(out + size - 1) = std::move(acc);
        } else {
          static_assert(Ival == interval::rhs, "else rhs, which has no right sibling that consumes the fold");
        }

        co_return;

      } else if constexpr (Ival == interval::mid || Ival == interval::rhs) { // A scan with left sibling.

        acc_t acc = acc_t(*(out - 1));

        LF_PRAGMA_UNROLL(8)
        for (; beg != end; ++beg, ++out) {
          if constexpr (async_bop) {
            co_await eager_call_outside(&acc, bop)(std::move(acc), co_await lf::just(proj)(*beg));
          } else {
            acc = std::invoke(bop, std::move(acc), co_await lf::just(proj)(*beg));
          }
          *out = acc;
        }

        co_return end;

      } else { // A scan with no left sibling.

        acc_t acc = acc_t(co_await lf::just(proj)(*beg));
        *out = acc;
        ++beg;
        ++out;

        LF_PRAGMA_UNROLL(8)
        for (; beg != end; ++beg, ++out) {
          if constexpr (async_bop) {
            co_await eager_call_outside(&acc, bop)(std::move(acc), co_await lf::just(proj)(*beg));
          } else {
            acc = std::invoke(bop, std::move(acc), co_await lf::just(proj)(*beg));
          }
          *out = acc;
        }

        co_return end;
      }
    }

    // Divide and recurse.
    int_t const mid = size / 2;

    /**
     * Specified through the following concept chain:
     *  random_access_iterator -> bidirectional_iterator -> forward_iterator -> incrementable -> regular
     */
    I left_out;

    if constexpr (Op == op::scan) {
      // If we are a scan then left child is a scan,
      static constexpr auto fork_sync = dispatch<tag::fork, modifier::sync_outside>;
      // Unconditionally launch left child (scan).
      auto ready = co_await fork_sync(&left_out, up_lhs{})(beg, beg + mid, n, bop, proj, out);
      // If the left child is ready and completely scanned then rhs can be a scan.
      // Otherwise the rhs must be a fold.
      if (ready && left_out == beg + mid) {
        // Effectively fused child trees into a single scan.
        // Right most scanned now from right child.
        co_return co_await lf::just(up_rhs<op::scan>{})(beg + mid, end, n, bop, proj, out + mid);
      }
    } else {
      co_await lf::fork(up_lhs{})(beg, beg + mid, n, bop, proj, out);
    }

    // clang-format off
    LF_TRY {
      co_await lf::call(up_rhs<op::fold>{})(beg + mid, end, n, bop, proj, out + mid);
    } LF_CATCH_ALL{
      self.stash_exception();
    }
    // clang-format on
    co_await lf::join;

    // Restore invariant: propagate the reduction (scan), to the right sibling (if we have one).
    if constexpr (Ival == interval::lhs || Ival == interval::mid) {

      O l_child = out + mid - 1;
      O r_child = out + size - 1;

      if constexpr (async_bop) {
        co_await eager_call_outside(r_child, bop)(*l_child, std::ranges::iter_move(r_child));
      } else {
        *r_child = std::invoke(bop, *l_child, std::ranges::iter_move(r_child));
      }
    }

    // If we are a scan then we return the end of the scanned range which was in the left child.
    if constexpr (Op == op::scan) {
      co_return left_out;
    }
  }
};

/**
 * As some of the input is always scanned during the reduction sweep, we always have a left sibling.
 */
template <std::random_access_iterator I, //
          std::sized_sentinel_for<I> S,  //
          class Proj,                    //
          class Bop,                     //
          std::random_access_iterator O, //
          interval Ival                  //
          >
  requires (Ival == interval::mid || Ival == interval::rhs)
struct fall_sweep_impl {
  /**
   * @brief The iterator difference type of I.
   */
  using int_t = std::iter_difference_t<I>;
  /**
   * @brief The accumulator type of the reduction.
   */
  using acc_t = std::iter_value_t<O>;
  /**
   * @brief Left child of the current interval.
   */
  using down_lhs = fall_sweep_impl<I, S, Proj, Bop, O, l_child_of(Ival)>;
  /**
   * @brief Right child of the current interval.
   */
  using down_rhs = fall_sweep_impl<I, S, Proj, Bop, O, r_child_of(Ival)>;
  /**
   * @brief If the binary operator is asynchronous, some optimizations can be done if it's not async.
   */
  static constexpr bool async_bop = async_invocable<Bop &, acc_t, acc_t>;
  /**
   * @brief Recursive implementation of `fall_sweep`, requires that `tail - head > 0`.
   */
  LF_STATIC_CALL auto
  operator()(auto self, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->lf::task<> {

    int_t const size = end - beg;

    LF_ASSERT(size > 0);

    static constexpr auto eager_call_outside = dispatch<tag::call, modifier::eager_throw_outside>;

    if (size <= n) { // Equivalent to a scan (maybe) with an initial value.

      acc_t acc = acc_t(*(out - 1));

      // The furthest-right chunk has no reduction stored in it so we include it in the scan.
      I last = (Ival == interval::rhs) ? end : beg + size - 1;

      LF_PRAGMA_UNROLL(8)
      for (; beg != last; ++beg, ++out) {
        if constexpr (async_bop) {
          co_await eager_call_outside(&acc, bop)(std::move(acc), co_await lf::just(proj)(*beg));
        } else {
          acc = std::invoke(bop, std::move(acc), co_await lf::just(proj)(*beg));
        }
        *out = acc;
      }
      co_return;
    }

    int_t const mid = size / 2;

    // Restore invariant: propagate the reduction (scan), we always have a left sibling.
    if constexpr (async_bop) {
      co_await eager_call_outside(out + mid - 1, bop)(*(out - 1), std::ranges::iter_move(out + mid - 1));
    } else {
      *(out + mid - 1) = std::invoke(bop, *(out - 1), std::ranges::iter_move(out + mid - 1));
    }

    // Divide and recurse.
    co_await lf::fork(down_lhs{})(beg, beg + mid, n, bop, proj, out);
    // clang-format off
    LF_TRY {
      co_await lf::call(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
    } LF_CATCH_ALL{
      self.stash_exception();
    }
    // clang-format on
    co_await lf::join;
  }
};

/**
 * As some of the input is always scanned during the reduction sweep, we always have a left sibling.
 */
template <std::random_access_iterator I, //
          std::sized_sentinel_for<I> S,  //
          class Proj,                    //
          class Bop,                     //
          std::random_access_iterator O, //
          interval Ival = interval::all  //
          >
struct fall_sweep {
  /**
   * @brief The iterator difference type of I.
   */
  using int_t = std::iter_difference_t<I>;
  /**
   * @brief Recursion to the left, computed interval.
   */
  using recur_lhs = fall_sweep<I, S, Proj, Bop, O, l_child_of(Ival)>;
  /**
   * @brief Recursion to the right, computed interval.
   */
  using recur_rhs = fall_sweep<I, S, Proj, Bop, O, r_child_of(Ival)>;
  /**
   * @brief Call the implementation, this can never be called on the left.
   */
  using down_rhs = fall_sweep_impl<I, S, Proj, Bop, O, r_child_of(Ival)>;
  /**
   * @brief Launch the implementation in the un-scanned chunks.
   *
   * Options:
   *  left fully scanned and right fully scanned -> return (make this impossible).
   *
   *  left fully scanned, right part-scanned -> (invar holds) recurse on right.
   *  left fully scanned, right un-scanned -> (invar holds), call fall_sweep on right.
   *  left part scanned, right un-scanned -> restore invar, recurse on left, call fall_sweep on right.
   */
  LF_STATIC_CALL auto operator()(auto self, I beg, S end, int_t n, Bop bop, Proj proj, O out, I scan_end)
      LF_STATIC_CONST->lf::task<> {

    LF_ASSERT(scan_end < end);

    for (;;) {

      int_t size = end - beg;
      int_t mid = size / 2;

      I split = beg + mid;

      if /*  */ (scan_end < split) {
        // Left part-scanned, right un-scanned.
        co_await lf::fork(recur_lhs{})(beg, beg + mid, n, bop, proj, out, scan_end);
        // clang-format off
        LF_TRY {  
            co_await lf::call(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
        } LF_CATCH_ALL{
          self.stash_exception();
        }
        // clang-format on
        co_await lf::join;
        co_return;
      } else if (scan_end == split) {
        // Left fully scanned, right un-scanned.
        co_return co_await lf::just(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
      } else if (scan_end > split) {
        // Left fully scanned, right part-scanned.
        if constexpr (!std::same_as<fall_sweep, recur_rhs>) {
          co_return co_await lf::just(recur_rhs{})(beg + mid, end, n, bop, proj, out + mid, scan_end);
        } else {
          // Recursion looks like, -^, hence we can loop.
          beg = beg + mid;
          out = out + mid;
          continue;
        }
      } else {
        unreachable();
      }
    }
  }
};

/**
 * @brief A wrapper around the rise/fall algorithms.
 */
struct scan_impl {
  /**
   * @brief Calls the rise_sweep and fall_sweep algorithms, checks for empty input.
   */
  template <std::random_access_iterator I, //
            std::sized_sentinel_for<I> S,  //
            std::random_access_iterator O, //
            class Proj,                    //
            class Bop                      //
            >
  LF_STATIC_CALL auto
  operator()(auto /* unused */, I beg, S end, O out, std::iter_difference_t<I> n, Bop bop, Proj proj)
      LF_STATIC_CONST->lf::task<> {

    // Early exit required if the input is empty.
    if (end == beg) {
      co_return;
    }

    constexpr rise_sweep<I, S, Proj, Bop, O> rise = {};
    constexpr fall_sweep<I, S, Proj, Bop, O> fall = {};

    // Up-sweep the reduction.
    I scan_end = co_await lf::just(rise)(beg, end, n, bop, proj, out);

    if (scan_end != end) {
      // If some un-scanned input remains, fall-sweep it.
      co_await lf::just(fall)(beg, end, n, bop, proj, out, scan_end);
    }
  }
};

/**
 * @brief Eight overloads of scan for (iterator/range, chunk/in_place, n = 1/n != 1).
 */
struct scan_overload {
  /**
   * @brief [iterator,chunk,output] version (5-6)
   */
  template <std::random_access_iterator I,                  //
            std::sized_sentinel_for<I> S,                   //
            std::random_access_iterator O,                  //
            class Proj = std::identity,                     //
            indirectly_scannable<O, projected<I, Proj>> Bop //
            >
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 O out,
                                 std::iter_difference_t<I> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<> {
    co_return co_await lf::just(impl::scan_impl{})(beg, end, out, n, bop, proj);
  }
  /**
   * @brief [iterator,n = 1,output] version (4-5)
   */
  template <std::random_access_iterator I,                  //
            std::sized_sentinel_for<I> S,                   //
            std::random_access_iterator O,                  //
            class Proj = std::identity,                     //
            indirectly_scannable<O, projected<I, Proj>> Bop //
            >
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 O out,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    co_return co_await lf::just(impl::scan_impl{})(beg, end, out, 1, bop, proj);
  }
  /**
   * @brief [iterator,chunk,in_place] version (4-5)
   */
  template <std::random_access_iterator I,                  //
            std::sized_sentinel_for<I> S,                   //
            class Proj = std::identity,                     //
            indirectly_scannable<I, projected<I, Proj>> Bop //
            >
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 std::iter_difference_t<I> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {

    co_return co_await lf::just(impl::scan_impl{})(beg, end, beg, n, bop, proj);
  }
  /**
   * @brief [iterator,n = 1,in_place] version.
   */
  template <std::random_access_iterator I,                  //
            std::sized_sentinel_for<I> S,                   //
            class Proj = std::identity,                     //
            indirectly_scannable<I, projected<I, Proj>> Bop //
            >
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 I beg,
                                 S end,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    co_return co_await lf::just(impl::scan_impl{})(beg, end, beg, 1, bop, proj);
  }
  /**
   * @brief [range,chunk,output] version (5-6)
   */
  template <std::ranges::random_access_range R,                                      //
            std::random_access_iterator O,                                           //
            class Proj = std::identity,                                              //
            indirectly_scannable<O, projected<std::ranges::iterator_t<R>, Proj>> Bop //
            >
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 O out,
                                 std::ranges::range_difference_t<R> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    co_return co_await lf::just(impl::scan_impl{})(
        std::ranges::begin(range), std::ranges::end(range), out, n, bop, proj //
    );
  }
  /**
   * @brief [range,n = 1,output] version (4-5)
   */
  template <std::ranges::random_access_range R,                                      //
            std::random_access_iterator O,                                           //
            class Proj = std::identity,                                              //
            indirectly_scannable<O, projected<std::ranges::iterator_t<R>, Proj>> Bop //
            >
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 O out,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    co_return co_await lf::just(impl::scan_impl{})(
        std::ranges::begin(range), std::ranges::end(range), out, 1, bop, proj //
    );
  }
  /**
   * @brief [range,chunk,in_place] version (4-5)
   */
  template <
      std::ranges::random_access_range R,                                                               //
      class Proj = std::identity,                                                                       //
      indirectly_scannable<std::ranges::iterator_t<R>, projected<std::ranges::iterator_t<R>, Proj>> Bop //
      >
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 std::ranges::range_difference_t<R> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    co_return co_await lf::just(impl::scan_impl{})(
        std::ranges::begin(range), std::ranges::end(range), std::ranges::begin(range), n, bop, proj //
    );
  }
  /**
   * @brief [range,n = 1,in_place] version.
   */
  template <
      std::ranges::random_access_range R,                                                               //
      class Proj = std::identity,                                                                       //
      indirectly_scannable<std::ranges::iterator_t<R>, projected<std::ranges::iterator_t<R>, Proj>> Bop //
      >
    requires std::ranges::sized_range<R>
  auto LF_STATIC_CALL operator()(auto /* unused */, //
                                 R &&range,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->task<void> {
    co_return co_await lf::just(impl::scan_impl{})(
        std::ranges::begin(range), std::ranges::end(range), std::ranges::begin(range), 1, bop, proj //
    );
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
