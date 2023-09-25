#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include "config.hpp"

namespace {

auto reduce(std::span<float> x, std::size_t grain_size) -> float {
  //
  if (x.size() <= grain_size) {
    return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  float a, b;

#pragma omp task untied shared(a, x, grain_size)
  a = reduce(x.first(h), grain_size);

  b = reduce(x.last(t), grain_size);

#pragma omp taskwait

  return a + b;
}

void reduce_omp(benchmark::State &state) {

  std::size_t n = state.range(0);
  std::vector data = to_sum();
  auto grain_size = data.size() / (10 * n);

  volatile float output;

  for (auto _ : state) {
#pragma omp parallel num_threads(n)
#pragma omp single
    output = reduce(data, grain_size);
  }
}

} // namespace

BENCHMARK(reduce_omp)->DenseRange(1, std::thread::hardware_concurrency())->UseRealTime();
