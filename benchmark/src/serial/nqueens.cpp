#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "nqueens.hpp"

import std;

namespace {

auto nqueens(int j, int n, char *a) -> std::int64_t {
  if (j == n) {
    return 1;
  }

  std::int64_t res = 0;

  for (int i = 0; i < n; ++i) {
    a[j] = static_cast<char>(i);
    if (queens_ok(j + 1, a)) {
      res += nqueens(j + 1, n, a);
    }
  }

  return res;
}

template <typename = void>
void nqueens_serial(benchmark::State &state) {

  int n = static_cast<int>(state.range(0));
  std::int64_t expect = nqueens_answers.at(static_cast<std::size_t>(n));

  state.counters["n"] = n;

  std::vector<char> board(static_cast<std::size_t>(n));

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = nqueens(0, n, board.data());
    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCH_ALL(nqueens_serial, serial, nqueens, nqueens)
