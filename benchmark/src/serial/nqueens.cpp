#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "nqueens.hpp"

import std;

namespace {

auto nqueens(int const *a, int n, int d, int i) -> std::int64_t {
  std::vector<int> aa(static_cast<std::size_t>(d + 1));

  for (int j = 0; j < d; ++j) {
    aa[static_cast<std::size_t>(j)] = a[j];

    int diff = a[j] - i;
    int dist = d - j;

    if (diff == 0 || dist == diff || dist + diff == 0) {
      return 0;
    }
  }

  if (d >= 0) {
    aa[static_cast<std::size_t>(d)] = i;
  }
  if (++d == n) {
    return 1;
  }

  std::int64_t res = 0;
  int const *next = aa.data();

  for (int col = 0; col < n; ++col) {
    res += nqueens(next, n, d, col);
  }

  return res;
}

template <typename = void>
void nqueens_serial(benchmark::State &state) {
  run_nqueens(state, [](int n) {
    return nqueens(nullptr, n, -1, 0);
  });
}

} // namespace

BENCH_ALL(nqueens_serial, serial, nqueens, nqueens)
