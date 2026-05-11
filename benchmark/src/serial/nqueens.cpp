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
  run_nqueens(state, [](int n, char *board) {
    return nqueens(0, n, board);
  });
}

} // namespace

BENCH_ALL(nqueens_serial, serial, nqueens, nqueens)
