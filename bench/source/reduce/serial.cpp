#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

void reduce_serial(benchmark::State &state) {

  state.counters["green_threads"] = 1;

  auto [data, exp] = get_data();

  state.counters["|reduce|"] = data.size();

  volatile double output;

  for (auto _ : state) {
    output = std::reduce(data.begin(), data.end());
  }
}

} // namespace

BENCHMARK(reduce_serial)->UseRealTime();
