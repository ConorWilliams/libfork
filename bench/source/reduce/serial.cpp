#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "config.hpp"

namespace {

void reduce_serial(benchmark::State &state) {

  std::vector data = to_sum();

  volatile float output;

  for (auto _ : state) {
    output = std::reduce(data.begin(), data.end());
  }
}

} // namespace

BENCHMARK(reduce_serial)->UseRealTime();
