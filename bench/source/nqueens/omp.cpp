#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto fib(int n) -> int {

  if (n < 2) {
    return n;
  }

  int x, y;

#pragma omp task untied shared(x) firstprivate(n) default(none)
  x = fib(n - 1);

  y = fib(n - 2);

#pragma omp taskwait

  return x + y;
}

template <std::size_t N>
auto nqueens(int j, std::array<char, N> const &a) -> int {

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
#pragma omp task untied shared(parts, buf) firstprivate(i, j) default(none)
      parts[i] = nqueens(j + 1, buf[i]);
    } else {
      parts[i] = 0;
    }
  }

#pragma omp taskwait

  return std::accumulate(parts.begin(), parts.end(), 0L);
}

void nqueens_omp(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["nqueens(n)"] = nqueens_work;

  std::size_t n = state.range(0);

  std::array<char, nqueens_work> buf{};

  volatile int output;

#pragma omp parallel num_threads(n)
#pragma omp single
  for (auto _ : state) {
    output = nqueens(0, buf);
  }

  if (output != answers[nqueens_work]) {
    std::cerr << "omp wrong answer: " << output << " != " << answers[nqueens_work] << std::endl;
  }
}

} // namespace

BENCHMARK(nqueens_omp)->Apply(targs)->UseRealTime();
