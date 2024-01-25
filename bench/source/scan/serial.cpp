#include <iostream>
#include <ranges>
#include <numeric>
#include <functional>

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
    std::inclusive_scan(in.begin(), in.end(), ou.begin(), std::plus<>{});
    sink = ou.back();
  }
}

} // namespace

BENCHMARK(scan_serial)->UseRealTime();