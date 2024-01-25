#include <iostream>
#include <ranges>
#include <numeric>
#include <functional>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void fold_serial(benchmark::State &state) {

  state.counters["fold(n)"] = fold_n;

  std::vector<unsigned> in = make_vec_fold();
  volatile unsigned sink = 0;

  for (auto _ : state) {
    sink = std::accumulate(in.begin(), in.end(), 0);
  }

  // std::cout << sink << std::endl;
}

} // namespace

BENCHMARK(fold_serial)->UseRealTime();