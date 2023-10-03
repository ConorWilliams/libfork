#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>

#include <benchmark/benchmark.h>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

template <std::size_t N>
auto nqueens(int j, std::array<char, N> const &a, tf::Subflow &sbf) -> long {

  if (N == j) {
    return 1;
  }

  long res = 0L;

  std::array<std::array<char, N>, N> buf;
  std::array<long, N> parts;

  for (int i = 0; i < N; i++) {

    for (int k = 0; k < j; k++) {
      buf[i][k] = a[k];
    }

    buf[i][j] = i;

    if (queens_ok(j + 1, buf[i].data())) {
      sbf.emplace([&parts, &buf, i, j](tf::Subflow &sbf) {
        parts[i] = nqueens(j + 1, buf[i], sbf);
      });
    } else {
      parts[i] = 0;
    }
  }

  sbf.join();

  return std::accumulate(parts.begin(), parts.end(), 0L);
}

void nqueens_taskflow(benchmark::State &state) {

  std::size_t n = state.range(0);
  tf::Executor executor(n);

  volatile int output;

  tf::Taskflow taskflow;

  std::array<char, nqueens_work> buf{};

  taskflow.emplace([&output, &buf](tf::Subflow &sbf) {
    output = nqueens(0, buf, sbf);
  });

  for (auto _ : state) {
    executor.run(taskflow).wait();
  }
}

} // namespace

BENCHMARK(nqueens_taskflow)->DenseRange(1, num_threads())->UseRealTime();
