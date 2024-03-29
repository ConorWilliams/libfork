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
auto nqueens(int j, std::array<char, N> const &a, tf::Subflow &sbf) -> int {

  if (N == j) {
    return 1;
  }

  int res = 0L;

  std::array<std::array<char, N>, N> buf;
  std::array<int, N> parts;

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

void nqueens_ztaskflow(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["nqueens(n)"] = nqueens_work;

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

  if (output != answers[nqueens_work]) {
    std::cerr << "taskflow wrong answer: " << output << " != " << answers[nqueens_work] << std::endl;
  }
}

} // namespace

BENCHMARK(nqueens_ztaskflow)->Apply(targs)->UseRealTime();
