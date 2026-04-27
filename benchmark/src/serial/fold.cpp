#include <benchmark/benchmark.h>

#include "fold.hpp"
#include "macros.hpp"

import std;

namespace {

template <typename = void>
void fold_serial(benchmark::State &state) {

  std::size_t n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["reps"] = fold_reps;

  std::vector<unsigned> in = fold_make_vec(n);
  std::uint64_t expect = fold_expected(n) * fold_reps;

  for (auto _ : state) {
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < fold_reps; ++i) {
      sum += std::accumulate(in.begin(), in.end(), std::uint64_t{0});
    }
    if (sum != expect) {
      state.SkipWithError(std::format("incorrect fold: {} != {}", sum, expect));
      break;
    }
    benchmark::DoNotOptimize(sum);
  }
}

} // namespace

BENCH_ALL(fold_serial, serial, fold, fold)
