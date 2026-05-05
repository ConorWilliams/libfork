#include <benchmark/benchmark.h>

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr int for_each_overhead_test = 1 << 10;
inline constexpr int for_each_overhead_base = 1 << 24;

inline constexpr int for_each_hetero_test = 1 << 8;
inline constexpr int for_each_hetero_base = 1 << 14;

struct trivial_inc {
  void operator()(std::int64_t &x) const {
    x += 1;
    benchmark::DoNotOptimize(x);
  }
};

struct hetero_work {
  void operator()(std::int64_t &x) const {
    std::int64_t iters = 64 + (x % 128) * 64;
    std::int64_t acc = 0;
    for (std::int64_t i = 0; i < iters; ++i) {
      acc += i * i;
      benchmark::DoNotOptimize(acc);
    }
    x = acc;
  }
};

template <typename Sch, typename Body>
void run_loop(benchmark::State &state, std::int64_t n, Body body) {
  state.counters["n"] = static_cast<double>(n);
  state.counters["p"] = static_cast<double>(thread_count<Sch>(state));
  state.SetComplexityN(static_cast<benchmark::IterationCount>(thread_count<Sch>(state)));

  Sch scheduler = make_scheduler<Sch>(state);

  std::vector<std::int64_t> v(static_cast<std::size_t>(n));
  std::iota(v.begin(), v.end(), 0);

  for (auto _ : state) {
    body(scheduler, v);
    benchmark::DoNotOptimize(v);
  }
}

// (1) Trivial work via overload (2): the n=1 specialised codepath.
template <lf::scheduler Sch>
void run_special(benchmark::State &state) {
  std::int64_t n = state.range(0);
  run_loop<Sch>(state, n, [](Sch &scheduler, std::vector<std::int64_t> &v) {
    lf::receiver recv = lf::schedule(scheduler, lf::for_each, v.begin(), v.end(), trivial_inc{});
    std::move(recv).get();
  });
}

// (2) Trivial work via overload (1) with n=1: the chunked codepath, no specialisation.
template <lf::scheduler Sch>
void run_chunked_n1(benchmark::State &state) {
  std::int64_t n = state.range(0);
  run_loop<Sch>(state, n, [](Sch &scheduler, std::vector<std::int64_t> &v) {
    lf::receiver recv =
        lf::schedule(scheduler, lf::for_each, v.begin(), v.end(), std::ptrdiff_t{1000}, trivial_inc{});
    std::move(recv).get();
  });
}

// (3) Heterogeneous per-element work via overload (1) with a real chunk size.
template <lf::scheduler Sch, std::ptrdiff_t Chunk>
void run_hetero(benchmark::State &state) {
  std::int64_t n = state.range(0);
  state.counters["chunk"] = static_cast<double>(Chunk);
  run_loop<Sch>(state, n, [](Sch &scheduler, std::vector<std::int64_t> &v) {
    lf::receiver recv = lf::schedule(scheduler, lf::for_each, v.begin(), v.end(), Chunk, hetero_work{});
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
