#include <benchmark/benchmark.h>

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr int for_each_overhead_test = 1 << 10;
inline constexpr int for_each_overhead_base = 1 << 24;

inline constexpr int for_each_hetero_test = 1 << 8;
inline constexpr int for_each_hetero_base = 1 << 14;

// Trivial body — exists only to be called. Takes by value so the iteration
// source can be a value-yielding view (e.g. std::views::iota), avoiding any
// backing storage so the benchmark measures framework overhead rather than
// memory bandwidth.
struct trivial_noop {
  void operator()(std::int64_t x) const { benchmark::DoNotOptimize(x); }
};

// Heterogeneous per-element work: iteration count varies with the element
// value, so different leaves do different amounts of compute. No memory
// traffic — `acc` is local and discarded via DoNotOptimize.
struct hetero_work {
  void operator()(std::int64_t x) const {
    std::int64_t iters = 64 + x * 64;
    std::int64_t acc = 0;
    for (std::int64_t i = 0; i < iters; ++i) {
      acc += i * i;
      benchmark::DoNotOptimize(acc);
    }
  }
};

template <typename Sch, typename Body>
void run_loop(benchmark::State &state, std::int64_t n, Body body) {
  state.counters["n"] = static_cast<double>(n);
  state.counters["p"] = static_cast<double>(thread_count<Sch>(state));
  state.SetComplexityN(static_cast<benchmark::IterationCount>(thread_count<Sch>(state)));

  Sch scheduler = make_scheduler<Sch>(state);

  auto range = std::views::iota(std::int64_t{0}, n);

  for (auto _ : state) {
    body(scheduler, range);
  }
}

// (1) Trivial work via overload (2): the n=1 specialised codepath.
template <lf::scheduler Sch>
void run_special(benchmark::State &state) {
  std::int64_t n = state.range(0);
  run_loop<Sch>(state, n, [](Sch &scheduler, auto &range) {
    lf::receiver recv = lf::schedule(scheduler, lf::for_each, range.begin(), range.end(), trivial_noop{});
    std::move(recv).get();
  });
}

// (2) Trivial work via overload (1): the chunked codepath.
template <lf::scheduler Sch>
void run_chunked_n1(benchmark::State &state) {
  std::int64_t n = state.range(0);
  run_loop<Sch>(state, n, [](Sch &scheduler, auto &range) {
    lf::receiver recv =
        lf::schedule(scheduler, lf::for_each, range.begin(), range.end(), std::ptrdiff_t{1}, trivial_noop{});
    std::move(recv).get();
  });
}

// (3) Heterogeneous per-element work via overload (1) with a real chunk size.
template <lf::scheduler Sch, std::ptrdiff_t Chunk>
void run_hetero(benchmark::State &state) {
  std::int64_t n = state.range(0);
  state.counters["chunk"] = static_cast<double>(Chunk);
  run_loop<Sch>(state, n, [](Sch &scheduler, auto &range) {
    lf::receiver recv =
        lf::schedule(scheduler, lf::for_each, range.begin(), range.end(), Chunk, hetero_work{});
    std::move(recv).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run_special, for_each_n1_special, for_each_overhead, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run_special, for_each_n1_special, for_each_overhead, poly_busy_pool)

LIBFORK_BENCH_ALL_MT(run_chunked_n1, for_each_n1_chunked, for_each_overhead, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run_chunked_n1, for_each_n1_chunked, for_each_overhead, poly_busy_pool)

LIBFORK_BENCH_ALL_MT(run_hetero, for_each_hetero, for_each_hetero, mono_busy_pool, 32)
LIBFORK_BENCH_ALL_MT(run_hetero, for_each_hetero, for_each_hetero, poly_busy_pool, 32)
