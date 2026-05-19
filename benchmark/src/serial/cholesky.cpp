#include <benchmark/benchmark.h>

#include "cholesky.hpp"
#include "macros.hpp"

import std;

namespace {

template <typename = void>
void cholesky_serial_bench(benchmark::State &state) {
  run_cholesky(state, [](cholesky_matrix &matrix, unsigned size) {
    cholesky_factor(size, matrix);
  });
}

} // namespace

BENCH_ALL(cholesky_serial_bench, serial, cholesky, cholesky)
