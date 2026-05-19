#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr int skynet_branching = 10;

// Tree depth: total leaves = branching ** depth.
inline constexpr int skynet_test = 4;
inline constexpr int skynet_base = 8;

constexpr auto skynet_leaves(int depth) -> std::int64_t {
  std::int64_t out = 1;
  for (int i = 0; i < depth; ++i) {
    out *= skynet_branching;
  }
  return out;
}

constexpr auto skynet_expected(int depth) -> std::int64_t {
  std::int64_t leaves = skynet_leaves(depth);
  return leaves * (leaves - 1) / 2;
}

template <typename Fn>
void run_skynet(benchmark::State &state, std::int64_t threads, Fn fn) {
  int depth = static_cast<int>(state.range(0));
  std::int64_t expect = skynet_expected(depth);

  state.counters["depth"] = depth;
  state.counters["leaves"] = static_cast<double>(skynet_leaves(depth));

  lf_bench::bench(state, threads, expect, [depth, fn]() -> std::int64_t {
    std::int64_t num = 0;
    int run_depth = depth;
    benchmark::DoNotOptimize(num);
    benchmark::DoNotOptimize(run_depth);
    return std::invoke(fn, num, run_depth);
  });
}

template <typename Fn>
void run_skynet(benchmark::State &state, Fn fn) {
  run_skynet(state, lf_bench::no_threads, fn);
}
