#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for operator<=, operator==, INTERNAL_CATCH_...
#include <concepts>                              // for constructible_from
#include <cstddef>                               // for size_t
#include <functional>                            // for plus, identity, multiplies
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

namespace {

enum class side {
  lhs, ///<
  rhs  ///<
};

template <side Tag>
struct vtag {};

inline constexpr std::size_t Fan = 2;

template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          class Proj,
          class Bop,
          std::random_access_iterator O,
          side Side = side::rhs>
struct reduction_sweep {

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;

  using reduction_lhs = reduction_sweep<I, S, Proj, Bop, O, side::lhs>;
  using reduction_rhs = reduction_sweep<I, S, Proj, Bop, O, Side>;

  /**
   * Requirements:
   *
   * Let J = projected<I, Proj>
   *
   * O must be indirectly readable.
   *
   * Let acc_t = iter_value_t<O>.
   * acc_t constructible_from indirect_value_t<J>.
   * indirect_value_t<J> convertible_to acc_t.
   * acc_t movable.
   * bop invocable with (acc_t, indirect_value_t<J>).
   * acc_t must be assignable from the result of -^.
   *
   *
   * O must be indirectly writable with acc_t.
   *
   * bop invocable with (iter_reference_t<O>, iter_rvalue_reference_t<O>).
   * O must be indirectly writable with the result of -^.
   *
   * iter_reference_t<O> must be convertible_to to acc_t.
   * acc_t must be constructible from iter_reference_t<O>.
   * O must be indirectly writable with acc_t &.
   */

  LF_STATIC_CALL auto
  operator()(auto /* */, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->lf::task<> {
    //
    int_t const size = end - beg;

    if (size <= n) { // Equivilent to a fold over acc_t.

      if constexpr (Side == side::rhs) {
        co_return;
      }

      acc_t acc = acc_t(proj(*beg));

      // The optimizer sometimes trips up here so we force a bit of unrolling.
#pragma unroll(8)
      for (++beg; beg != end; ++beg) {
        acc = bop(std::move(acc), proj(*beg));
      }

      *(out + size - 1) = std::move(acc);

      co_return;
    }

    int_t const mid = size / 2;

    // Divide and recurse.
    co_await lf::fork(reduction_lhs{})(beg, beg + mid, n, bop, proj, out);
    co_await lf::call(reduction_rhs{})(beg + mid, end, n, bop, proj, out + mid);
    co_await lf::join;

    //  Propagete the reduction (scan).
    if constexpr (Side == side::lhs) {
      *(out + size - 1) = bop(*(out + mid - 1), std::ranges::iter_move(out + size - 1));
    }
  };
};

template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          class Proj,
          class Bop,
          std::random_access_iterator O,
          side Side = side::lhs>
struct scan_sweep {

  using int_t = std::iter_difference_t<I>;
  using acc_t = std::iter_value_t<O>;

  using sweep_lhs = scan_sweep<I, S, Proj, Bop, O, Side>;
  using sweep_rhs = scan_sweep<I, S, Proj, Bop, O, side::rhs>;

  /**
   * Additional requirements:
   *
   * iter_reference_t<O> must be convertible_to to acc_t.
   * acc_t must be constructible from iter_reference_t<O>.
   * O must be indirectly writable with acc_t &.
   *
   *
   */

  LF_STATIC_CALL auto
  operator()(auto /* */, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->lf::task<> {

    int_t const size = end - beg;

    if (size <= n) { // Equivilent to a scan (maybe) with an initial value.

      acc_t acc = [&]() -> acc_t {
        if constexpr (Side == side::rhs) {
          return *(out - 1);
        } else {
          return proj(*beg);
        }
      }();

      if constexpr (Side == side::rhs) {
        *out = acc;
        ++beg;
        ++out;
      }

      // The optimizer sometimes trips up here so we force a bit of unrolling.
#pragma unroll(8)
      for (I last = beg + size - 1; beg != last; ++beg, ++out) {
        *out = acc = bop(std::move(acc), proj(*beg));
      }

      co_return;
    }

    int_t const mid = size / 2;

    // Propagate the reduction (scan).
    if constexpr (Side == side::rhs) {
      *(out + mid - 1) = bop(*(out - 1), std::ranges::iter_move(out + mid - 1));
    }

    // Divide and recurse.
    co_await lf::fork(sweep_lhs{})(beg, beg + mid, n, bop, proj, out);
    co_await lf::call(sweep_rhs{})(beg + mid, end, n, bop, proj, out + mid);
    co_await lf::join;
  }
};

constexpr auto repeat_it = []<class I, class S, class O>(auto, I beg, S end, O out) -> lf::task<void> {
  for (std::size_t i = 0; i < scan_reps; ++i) {
    // std::inclusive_scan(in, in + scan_n, ou, std::plus<>{}); ///
    co_await lf::just(reduction_sweep<I, S, std::identity, std::plus<>, O>{})(
        beg, end, scan_chunk, std::plus<>{}, std::identity{}, out);

    co_await lf::just(scan_sweep<I, S, std::identity, std::plus<>, O>{})(
        beg, end, scan_chunk, std::plus<>{}, std::identity{}, out);

    // *(out + size - 1) = bop(*(out + size - 2), *(in + size - 1), )
  }
  co_return;
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void scan_libfork2(benchmark::State &state) {

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

BENCHMARK(scan_libfork2<lf::lazy_pool, lf::numa_strategy::fan>)->Apply(targs)->UseRealTime();
