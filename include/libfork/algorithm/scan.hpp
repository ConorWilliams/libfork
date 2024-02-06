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
#include "libfork/core/control_flow.hpp"     // for call, fork, join
#include "libfork/core/just.hpp"             // for just
#include "libfork/core/macro.hpp"            // for LF_STATIC_CALL, LF_STATIC_CONST, LF_CATCH_ALL
#include "libfork/core/tag.hpp"
#include "libfork/core/task.hpp" // for task

/**
 * @file scan.hpp
 *
 * @brief Implementation of a parallel scan.
 */

namespace lf {

namespace v2 {

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

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;
  using task_t = lf::task<std::conditional_t<Op == op::scan, I, void>>;

  // Propagate the operation to the left child.
  using up_lhs = rise_sweep<I, S, Proj, Bop, O, l_child_of(Ival), Op>;

  // The right child's operation depends on the readiness of the left child.
  template <op OpChild>
  using up_rhs = rise_sweep<I, S, Proj, Bop, O, r_child_of(Ival), OpChild>;

  /**
   * Return the end of the scanned range.
   */
  LF_STATIC_CALL auto
  operator()(auto /* */, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->task_t {
    //
    int_t const size = end - beg;

    if (size < 1) {
      throw "poop";
    }

    if (size <= n) {
      if constexpr (Op == op::fold) { // Equivalent to a fold over acc_t, left sibling not ready.

        static_assert(Ival != interval::lhs && Ival != interval::all, "left can always scan");

        // std::cout << "fold" << '\n';

        if constexpr (Ival == interval::mid) {
          // Mid segment has a right sibling so do the fold.
          acc_t acc = acc_t(proj(*beg));
          // The optimizer sometimes trips up here so we force a bit of unrolling.
#pragma unroll(8)
          for (++beg; beg != end; ++beg) {
            acc = bop(std::move(acc), proj(*beg));
          }
          // Store in the correct location in the output (in-case the scan is in-place).
          *(out + size - 1) = std::move(acc);
        } else {
          static_assert(Ival == interval::rhs, "else rhs, which has no right sibling that consumes the fold");
        }

        co_return;

      } else { // A scan implies the left sibling (if it exists) is ready.

        // std::transform_inclusive_scan(beg, end, out, bop, proj);

        constexpr bool has_left_carry = Ival == interval::mid || Ival == interval::rhs;

        // std::cout << "has_left_carry: " << has_left_carry << '\n';

        acc_t acc = [&]() -> acc_t {
          if constexpr (has_left_carry) {
            return *(out - 1);
          } else {
            return proj(*beg);
          }
        }();

        if constexpr (!has_left_carry) {
          *out = acc;
          ++beg;
          ++out;
        }

        // The optimizer sometimes trips up here so we force a bit of unrolling.
#pragma unroll(8)
        for (; beg != end; ++beg, ++out) {
          *out = acc = bop(std::move(acc), proj(*beg));
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

      // Unconditionally launch left child (scan).
      using mod = lf::modifier::sync_outside;
      using lf::tag::fork;
      auto ready = co_await lf::dispatch<fork, mod>(&left_out, up_lhs{})(beg, beg + mid, n, bop, proj, out);

      // auto r2 = rand() % 2 == 0;

      // std::cout << "lhs ready: " << r2 << '\n';

      // If the left child is ready and completely scanned then rhs can be a scan.
      // Otherwise the rhs must be a fold.
      if (ready && left_out == beg + mid) {
        // Effectively fused child trees into a single scan.
        // Right most scanned now from right child.
        co_return co_await lf::just(up_rhs<op::scan>{})(beg + mid, end, n, bop, proj, out + mid);
      }
    } else {
      // std::cout << "fold-tree\n";
      co_await lf::fork(up_lhs{})(beg, beg + mid, n, bop, proj, out);
    }

    co_await lf::call(up_rhs<op::fold>{})(beg + mid, end, n, bop, proj, out + mid);
    co_await lf::join;

    // Restore invariant: propagate the reduction (scan), to the right sibling (if we have one).
    if constexpr (Ival == interval::lhs || Ival == interval::mid) {
      *(out + size - 1) = bop(*(out + mid - 1), std::ranges::iter_move(out + size - 1));
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

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;

  using down_lhs = fall_sweep_impl<I, S, Proj, Bop, O, l_child_of(Ival)>;
  using down_rhs = fall_sweep_impl<I, S, Proj, Bop, O, r_child_of(Ival)>;

  LF_STATIC_CALL auto
  operator()(auto /* */, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->lf::task<> {

    int_t const size = end - beg;

    if (size < 1) {
      throw "poop";
    }

    if (size <= n) { // Equivalent to a scan (maybe) with an initial value.

      acc_t acc = acc_t(*(out - 1));

      // The furthest-right chunk has no reduction stored in it so we include it in the scan.
      I last = (Ival == interval::rhs) ? end : beg + size - 1;

      // The optimizer sometimes trips up here so we force a bit of unrolling.
#pragma unroll(8)
      for (; beg != last; ++beg, ++out) {
        *out = acc = bop(std::move(acc), proj(*beg));
      }

      co_return;
    }

    int_t const mid = size / 2;

    // Restore invariant: propagate the reduction (scan), we always have a left sibling.
    *(out + mid - 1) = bop(*(out - 1), std::ranges::iter_move(out + mid - 1));

    // Divide and recurse.
    co_await lf::fork(down_lhs{})(beg, beg + mid, n, bop, proj, out);
    co_await lf::call(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
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

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;

  using recur_lhs = fall_sweep<I, S, Proj, Bop, O, l_child_of(Ival)>;
  using recur_rhs = fall_sweep<I, S, Proj, Bop, O, r_child_of(Ival)>;

  using down_rhs = fall_sweep_impl<I, S, Proj, Bop, O, r_child_of(Ival)>;

  /**
   * Options:
   *  left fully scanned and right fully scanned -> return (make this impossible).
   *
   *  left fully scanned, right part-scanned -> (invar holds) recurse on right.
   *  left fully scanned, right un-scanned -> (invar holds), call fall_sweep on right.
   *  left part scanned, right un-scanned -> restore invar, recurse on left, call fall_sweep on right.
   */
  LF_STATIC_CALL auto
  operator()(auto /* unused */, I beg, S end, int_t n, Bop bop, Proj proj, O out, I scan_end)
      LF_STATIC_CONST->lf::task<> {

    if (end <= scan_end) {
      throw "should be impossible";
    }

    for (;;) {

      int_t size = end - beg;
      int_t mid = size / 2;

      I split = beg + mid;

      if /*  */ (scan_end < split) {

        // std::cout << "split less\n";

        // Left part-scanned, right un-scanned.

        // // Restore invariant: propagate the reduction (scan), if we have a left sibling.
        // if constexpr (Ival == interval::mid || Ival == interval::rhs) {
        //   *(out + mid - 1) = bop(*(out - 1), std::ranges::iter_move(out + mid - 1));
        // }

        co_await lf::fork(recur_lhs{})(beg, beg + mid, n, bop, proj, out, scan_end);
        co_await lf::call(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
        co_await lf::join;
        co_return;
      } else if (scan_end == split) {
        // std::cout << "split equal\n";
        // Left fully scanned, right un-scanned.
        co_return co_await lf::just(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
      } else if (scan_end > split) {
        // std::cout << "split greater\n";
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
        lf::impl::unreachable();
      }
    }
  }
};

/**
 * @brief Calls the rise_sweep and fall_sweep algorithms, checks for empty input.
 */
template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          std::random_access_iterator O,
          class Proj,
          class Bop //
          >
struct scan_impl {

  using int_t = std::iter_difference_t<I>;

  static constexpr rise_sweep<I, S, Proj, Bop, O> rise = {};
  static constexpr fall_sweep<I, S, Proj, Bop, O> fall = {};

  LF_STATIC_CALL auto
  operator()(auto /**/, I beg, S end, O out, int_t n, Bop bop, Proj proj) LF_STATIC_CONST->lf::task<> {

    // Early exit required if the input is empty.
    if (end == beg) {
      co_return;
    }

    // Up-sweep the reduction.
    I scan_end = co_await lf::just(rise)(beg, end, n, bop, proj, out);

    // std::cout << "scanned: " << scan_end - beg << '\n';

    // std::cout << "up: ";

    for (auto it = out; it != out + (end - beg); ++it) {
      // std::cout << *it << ' ';
    }

    // std::cout << '\n';

    if (scan_end != end) {
      // If some un-scanned input remains, fall-sweep it.
      co_await lf::just(fall)(beg, end, n, bop, proj, out, scan_end);
    }
  }
};

} // namespace v2

namespace impl {

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

#pragma unroll(8)
    for (++beg; beg != end; ++beg) {

      auto prev = out;
      ++out;

      if constexpr (InPlace) {
        LF_ASSERT(out == beg.base());
      }

      using mod = modifier::eager_throw_outside;

      if constexpr (async_bop) {
        co_await lf::dispatch<tag::call, mod>(out, bop)(*prev, co_await just(proj)(*beg));
      } else {
        *out = std::invoke(bop, *prev, co_await just(proj)(*beg));
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

  using mod = modifier::eager_throw_outside;

  if constexpr (async_bop) {
    co_await lf::dispatch<tag::call, mod>(r_child, bop)(*l_child, std::ranges::iter_move(r_child));
  } else {
    *r_child = bop(*l_child, std::ranges::iter_move(r_child));
  }
};

/**
 * @brief The down sweep of the scan.
 *
 * Here we recurse from the root to the leaves, at each stage if the sub-tree has a sibling to the left
 * we merge the reduction into the left child preserving the invariant that the left child has the
 prefix-sum
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
inline constexpr auto rhs_fall_sweep =
    []<std::random_access_iterator O, std::sized_sentinel_for<O> S, typename Bop>(auto rhs_fall_sweep, //
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

  using mod = modifier::eager_throw_outside;

  if (size <= n) {
#pragma unroll(8)
    for (; beg != end - 1; ++beg) {
      if constexpr (async_bop) {
        co_await lf::dispatch<tag::call, mod>(beg, bop)(*acc_prev, std::ranges::iter_move(beg));
      } else {
        *beg = bop(*acc_prev, std::ranges::iter_move(beg));
      }
    }
    co_return;
  }

  std::iter_difference_t<O> half = size / 2; //
  O rhs = beg + (half - 1);                  // This is the left child of the tree.

  if constexpr (async_bop) {
    co_await lf::dispatch<tag::call, mod>(rhs, bop)(*acc_prev, std::ranges::iter_move(rhs));
  } else {
    *rhs = bop(*acc_prev, std::ranges::iter_move(rhs));
  }

  O mid = beg + half;

  // clang-format off

  LF_TRY {
    co_await lf::fork(rhs_fall_sweep)(beg, mid, n, bop);
    co_await lf::call(rhs_fall_sweep)(mid, end, n, bop);
  } LF_CATCH_ALL {
    rhs_fall_sweep.stash_exception();
  }

  // clang-format on

  co_await lf::join;
};

/**
 * @brief Down-sweep of sub-tree with no left sibling.
 */
inline constexpr auto lhs_fall_sweep =
    []<std::random_access_iterator O, std::sized_sentinel_for<O> S, typename Bop>(auto lhs_fall_sweep, //
                                                                                  O beg,
                                                                                  S end,
                                                                                  std::iter_difference_t<O> n,
                                                                                  Bop bop)
        LF_STATIC_CALL -> task<void> {
  if (auto size = end - beg; size > n) {

    auto mid = beg + size / 2;

    // clang-format off
     
        LF_TRY {
          co_await lf::fork(lhs_fall_sweep)(beg, mid, n, bop);
          co_await lf::call(rhs_fall_sweep)(mid, end, n, bop);
        } LF_CATCH_ALL {
          lhs_fall_sweep.stash_exception();
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
                                 Proj proj = {}) LF_STATIC_CONST->task<> {

    co_return co_await lf::just(v2::scan_impl<I, S, O, Proj, Bop>{})(beg, end, out, n, bop, proj);

    // if (std::iter_difference_t<I> size = end - beg; size > 0) {
    //   co_await lf::just(reduction_sweep<false>)(beg, end, n, bop, proj, out);
    //   co_await lf::just(lhs_fall_sweep)(out, out + size, n, bop);
    // }
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
      co_await lf::just(lhs_fall_sweep)(out, out + size, std::iter_difference_t<I>(1), bop);
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
      co_await lf::just(lhs_fall_sweep)(beg, end, n, bop);
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
      co_await lf::just(lhs_fall_sweep)(beg, end, std::iter_difference_t<I>(1), bop);
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
      co_await lf::just(lhs_fall_sweep)(out, out + std::ranges::ssize(range), n, bop);
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
      co_await lf::just(lhs_fall_sweep)(
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
      co_await lf::just(lhs_fall_sweep)(std::ranges::begin(range), std::ranges::end(range), n, bop);
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
      co_await lf::just(lhs_fall_sweep)(
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
