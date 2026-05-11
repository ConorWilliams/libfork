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
inline constexpr int skynet_test = 4; // 10^4 = 10'000 leaves
inline constexpr int skynet_base = 6; // 10^6 = 1'000'000 leaves

inline constexpr auto skynet_leaves(int depth) -> std::int64_t {
  std::int64_t out = 1;
  for (int i = 0; i < depth; ++i) {
    out *= skynet_branching;
  }
  return out;
}

inline constexpr auto skynet_expected(int depth) -> std::int64_t {
  std::int64_t leaves = skynet_leaves(depth);
  return leaves * (leaves - 1) / 2;
}

template <typename Fn>
void run_skynet(benchmark::State &state, Fn fn) {
  int depth = static_cast<int>(state.range(0));
  std::int64_t expect = skynet_expected(depth);

  state.counters["depth"] = depth;
  state.counters["leaves"] = static_cast<double>(skynet_leaves(depth));

  lf_bench::bench(state, expect, [depth, fn]() -> std::int64_t {
    return std::invoke(fn, 0, depth);
  });
}
