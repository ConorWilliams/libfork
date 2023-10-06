#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

template <std::size_t N>
auto nqueens(int j, std::array<char, N> const &a) -> long {

  if (N == j) {
    return 1;
  }

  long res = 0L;

  for (int i = 0; i < N; i++) {

    std::array<char, N> b;

    for (int k = 0; k < j; k++) {
      b[k] = a[k];
    }

    b[j] = i;

    if (queens_ok(j + 1, b.data())) {
      res += nqueens(j + 1, b);
    }
  }

  return res;
}

void nqueens_serial(benchmark::State &state) {

  state.counters["green_threads"] = 1;
  state.counters["nqueens(n)"] = nqueens_work;

  volatile int output;

  std::array<char, 13> buf{};

  for (auto _ : state) {
    output = nqueens(0, buf);
  }

  if (output != answers[nqueens_work]) {
    std::cerr << "serial wrong answer: " << output << " != " << answers[nqueens_work] << std::endl;
  }
}

} // namespace

BENCHMARK(nqueens_serial)->UseRealTime();
