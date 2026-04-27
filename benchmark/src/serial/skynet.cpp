#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "skynet.hpp"

import std;

namespace {

auto skynet_recurse(std::int64_t num, int depth) -> std::int64_t {
  if (depth == 0) {
    return num;
  }

  std::int64_t sub = skynet_leaves(depth - 1);
  std::int64_t sum = 0;
  for (int i = 0; i < skynet_branching; ++i) {
    sum += skynet_recurse(num + i * sub, depth - 1);
  }
  return sum;
}

template <typename = void>
void skynet_serial(benchmark::State &state) {

  int depth = static_cast<int>(state.range(0));
  std::int64_t expect = skynet_expected(depth);
  state.counters["depth"] = depth;
  state.counters["leaves"] = static_cast<double>(skynet_leaves(depth));

  for (auto _ : state) {
    benchmark::DoNotOptimize(depth);
    std::int64_t result = skynet_recurse(0, depth);
    if (result != expect) {
      state.SkipWithError(std::format("skynet sum mismatch: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCH_ALL(skynet_serial, serial, skynet, skynet)
