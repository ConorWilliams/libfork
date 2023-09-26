#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include "../util.hpp"
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

#pragma omp parallel num_threads(n)
#pragma omp single
  {
    std::vector<float> data;

#pragma omp task shared(data) // No untied
    data = to_sum();

#pragma omp taskwait

    auto grain_size = data.size() / (n * 10);

    volatile float output;

    for (auto _ : state) {
      output = reduce(data, grain_size);
    }
  }
}

} // namespace

BENCHMARK(reduce_omp)->DenseRange(1, num_threads())->UseRealTime();
