#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>

#include <benchmark/benchmark.h>

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

template <std::size_t N>
auto nqueens(int j, std::array<char, N> const &a) -> int {

  if (N == j) {
    return 1;
  }

  int res = 0L;

  std::array<std::array<char, N>, N> buf;
  std::array<int, N> parts;

  tbb::task_group g;

  for (int i = 0; i < N; i++) {

    for (int k = 0; k < j; k++) {
      buf[i][k] = a[k];
    }

    buf[i][j] = i;

    if (queens_ok(j + 1, buf[i].data())) {
      g.run([&parts, &buf, i, j] {
        parts[i] = nqueens(j + 1, buf[i]);
      });
    } else {
      parts[i] = 0;
    }
  }

  g.wait();

  return std::accumulate(parts.begin(), parts.end(), 0L);
}

void nqueens_tbb(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["nqueens(n)"] = nqueens_work;

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);

  volatile int output;

  std::array<char, nqueens_work> buf{};

  for (auto _ : state) {
    output = arena.execute([&] {
      return nqueens(0, buf);
    });
  }

  if (output != answers[nqueens_work]) {
    std::cerr << "tbb wrong answer: " << output << " != " << answers[nqueens_work] << std::endl;
  }
}

} // namespace

BENCHMARK(nqueens_tbb)->Apply(targs)->UseRealTime();
