#include <functional>
#include <iostream>
#include <numeric>
#include <ranges>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void scan_serial(benchmark::State &state) {

  state.counters["scan(n)"] = scan_n;

  std::vector<unsigned> in = make_vec();
  std::vector<unsigned> ou(in.size());

  volatile unsigned sink = 0;

  for (auto _ : state) {
    for (std::size_t i = 0; i < scan_reps; ++i) {
      std::inclusive_scan(in.begin(), in.end(), ou.begin(), std::plus<>{});
      std::ranges::swap(in, ou);
      std::inclusive_scan(ou.begin(), ou.end(), in.begin(), std::plus<>{});
      std::ranges::swap(in, ou);
    }
  }

  sink = ou.back();
}

} // namespace

BENCHMARK(scan_serial)->UseRealTime();