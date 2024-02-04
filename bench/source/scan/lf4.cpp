#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for operator<=, operator==, INTERNAL_CATCH_...
#include <compare>
#include <concepts>   // for constructible_from
#include <cstddef>    // for size_t
#include <functional> // for plus, identity, multiplies
#include <iostream>
#include <iterator>
#include <limits>  // for numeric_limits
#include <numeric> // for inclusive_scan
#include <random>  // for random_device, uniform_int_distribution
#include <stdexcept>
#include <string>      // for operator+, string, basic_string
#include <thread>      // for thread
#include <type_traits> // for type_identity
#include <utility>     // for forward
#include <vector>      // for operator==, vector

#include <benchmark/benchmark.h>

#include "libfork.hpp"

#include "../util.hpp"
#include "config.hpp"
#include "libfork/core/control_flow.hpp"
#include "libfork/core/tag.hpp"

namespace {

enum class side {
  lhs, ///< Always scan on the left hand side.
  rhs  ///< We have no right sibling so we don't need to do anything.
};

/**
 * Operation propagates as:
 *
 * scan -> (scan, x)
 *    x can be scan if the left child completes syncronously, otherwise it must be a fold.
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

template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          class Proj,
          class Bop,
          std::random_access_iterator O,
          interval Ival = interval::all,
          op Op = op::scan //
          >
struct up_sweep {

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;
  using task_t = lf::task<std::conditional_t<Op == op::scan, I, void>>;

  // Propagate the operation to the left child.
  using up_lhs = up_sweep<I, S, Proj, Bop, O, l_child_of(Ival), Op>;

  // The right child's operation depends on the readiness of the left child.
  template <op OpChild>
  using up_rhs = up_sweep<I, S, Proj, Bop, O, r_child_of(Ival), OpChild>;

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

        if constexpr (Ival == interval::mid) {
          // Mid segement has a right sibling so do the fold.
          acc_t acc = acc_t(proj(*beg));
          // The optimizer sometimes trips up here so we force a bit of unrolling.
#pragma unroll(8)
          for (++beg; beg != end; ++beg) {
            acc = bop(std::move(acc), proj(*beg));
          }
          *(out + size - 1) = std::move(acc);
        } else {
          static_assert(Ival == interval::rhs, "rhs has no right sibling that consumes fold");
        }

        co_return;

      } else { // A scan implies the left sibling (if it exists) is ready.

        constexpr bool has_left_carry = Ival == interval::mid || Ival == interval::rhs;

        acc_t acc = [&]() -> acc_t {
          if constexpr (has_left_carry) {
            return *(out - 1);
          } else {
            return proj(*beg);
          }
        }();

        if constexpr (has_left_carry) {
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
      // Unconditionally launch left child.

      using mod = lf::modifier::sync_outside;

      // If we are a scan then left child is a scan,
      // If the left child is ready then rhs can be a scan.
      // Otherwise the rhs must be a fold.
      if (co_await lf::dispatch<lf::tag::fork, mod>(&left_out, up_lhs{})(beg, beg + mid, n, bop, proj, out)) {
        // TODO: needs to handle exceptions from fork!

        // Effectively fused child trees into a single scan.
        // Right most scanned now from right child.
        co_return co_await lf::just(up_rhs<op::scan>{})(beg + mid, end, n, bop, proj, out + mid);
      }
    } else {
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
template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          class Proj,
          class Bop,
          std::random_access_iterator O,
          interval Ival = interval::mid //
          >
  requires (Ival == interval::mid || Ival == interval::rhs)
struct down_sweep_impl {

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;

  using down_lhs = down_sweep_impl<I, S, Proj, Bop, O, l_child_of(Ival)>;
  using down_rhs = down_sweep_impl<I, S, Proj, Bop, O, r_child_of(Ival)>;

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
template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          class Proj,
          class Bop,
          std::random_access_iterator O,
          interval Ival = interval::mid //
          >
  requires (Ival == interval::mid || Ival == interval::rhs)
struct down_sweep {

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;

  using recur_lhs = down_sweep<I, S, Proj, Bop, O, l_child_of(Ival)>;
  using down_rhs = down_sweep_impl<I, S, Proj, Bop, O, r_child_of(Ival)>;

  /**
   * Options:
   *  left fully scanned and right fully scanned -> return (make this impossible).
   *
   *  left fully scanned, right part-scanned -> (invar holds) recurse on right.
   *  left fully scanned, right un-scanned -> (invar holds), call down_sweep on right.
   *  left part scanned, right un-scanned -> restore invar, recurse on left, call down_sweep on right.
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
        // Left part-scanned, right un-scanned.

        // Restore invariant: propagate the reduction (scan), we always have a left sibling.
        *(out + mid - 1) = bop(*(out - 1), std::ranges::iter_move(out + mid - 1));

        co_await lf::fork(recur_lhs{})(beg, beg + mid, n, bop, proj, out, scan_end);
        co_await lf::call(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
        co_await lf::join;
        co_return;
      } else if (scan_end == split) {
        // Left fully scanned, right un-scanned.
        co_return co_await lf::just(down_rhs{})(beg + mid, end, n, bop, proj, out + mid);
      } else if (scan_end > split) {
        // Left fully scanned, right part-scanned.

        // Recursion looks like: call(recur_rhs{})(beg + mid, end, n, bop, proj, out + mid, scan_end)
        // Hence, we can loop.

        static_assert(Ival == r_child_of(Ival), "recursion into different function");

        beg = beg + mid;
        out = out + mid;

        continue;
      } else {
        lf::impl::unreachable();
      }
    }
  }
};

constexpr auto repeat_it = []<class I, class S, class O>(auto, I beg, S end, O out) -> lf::task<void> {
  for (std::size_t i = 0; i < scan_reps; ++i) {

    using up = up_sweep<I, S, std::identity, std::plus<>, O>;
    using down = down_sweep<I, S, std::identity, std::plus<>, O>;

    // std::inclusive_scan(in, in + scan_n, ou, std::plus<>{}); ///
    I scan = co_await lf::just(up{})(beg, end, scan_chunk, std::plus<>{}, std::identity{}, out);

    if (scan != end) {
      co_await lf::just(down{})(beg, end, scan_chunk, std::plus<>{}, std::identity{}, out, scan);
    }

    // std::cout << "scan: " << scan - beg << std::endl;

    // co_await lf::just(down_sweep<I, S, std::identity, std::plus<>, O>{})(
    //     beg, end, scan_chunk, std::plus<>{}, std::identity{}, out);

    // *(out + size - 1) = bop(*(out + size - 2), *(in + size - 1), )
  }
  co_return;
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void scan_libfork4(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["n"] = scan_n;
  state.counters["reps"] = scan_reps;
  state.counters["chunk"] = scan_chunk;

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  std::vector in = lf::sync_wait(sch, lf::lift, make_vec);

  std::vector ou = lf::sync_wait(sch, lf::lift, [&] {
    return std::vector{in};
  });

  volatile unsigned sink = 0;

  for (auto _ : state) {
    lf::sync_wait(sch, repeat_it, in.begin(), in.end(), ou.begin());
  }

  sink = ou.back();
}

} // namespace

BENCHMARK(scan_libfork4<lf::lazy_pool, lf::numa_strategy::fan>)->Apply(targs)->UseRealTime();
