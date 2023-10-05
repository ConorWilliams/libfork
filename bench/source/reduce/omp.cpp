#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto reduce(std::span<double> x, std::size_t grain_size) -> double {
  //
  if (x.size() <= grain_size) {
    return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  double a, b;

#pragma omp task untied shared(a, x, grain_size, h) default(none)
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
    std::vector<double> data;

#pragma omp task shared(data) // No untied
    data = to_sum();

#pragma omp taskwait

    auto grain_size = data.size() / (n * 10);

    volatile double output;

    for (auto _ : state) {
      output = reduce(data, grain_size);
    }

    if (auto exp = std::reduce(data.begin(), data.end()); !is_close(output, exp)) {
      std::cerr << "omp wrong result: " << output << " != " << exp << std::endl;
    }
  }
}

} // namespace

BENCHMARK(reduce_omp)->DenseRange(1, num_threads())->UseRealTime();
