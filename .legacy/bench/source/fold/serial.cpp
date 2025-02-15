#include <functional>
#include <iostream>
#include <numeric>
#include <ranges>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void fold_serial(benchmark::State &state) {

  state.counters["fold(n)"] = fold_n;

  std::vector<unsigned> in = make_vec_fold();
  volatile unsigned sink = 0;

  unsigned sum = 0;

  for (auto _ : state) {
    for (std::size_t i = 0; i < fold_reps; ++i) {
      sum += std::accumulate(in.begin(), in.end(), 0);
    }
  }

  sink = sum;

  // std::cout << sink << std::endl;
}

} // namespace

BENCHMARK(fold_serial)->UseRealTime();