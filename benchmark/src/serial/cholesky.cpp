#include <benchmark/benchmark.h>

#include "cholesky.hpp"
#include "macros.hpp"

import std;

namespace {

void cholesky_serial(double *A, unsigned n, unsigned s) {
  if (n <= cholesky_cutoff) {
    cholesky_basecase(A, n, s);
    return;
  }

  unsigned m = n / 2;
  double *A00 = A;
  double *A10 = A + static_cast<std::size_t>(m) * s;
  double *A11 = A + static_cast<std::size_t>(m) * s + m;

  cholesky_serial(A00, m, s);
  cholesky_solve_rows(A10, A00, 0, m, m, s);
  cholesky_schur_rows(A11, A10, 0, m, m, s);
  cholesky_serial(A11, m, s);
}

template <typename = void>
void cholesky_serial_bench(benchmark::State &state) {
  run_cholesky(state, [](double *A, unsigned n, unsigned s) {
    cholesky_serial(A, n, s);
  });
}

} // namespace

BENCH_ALL(cholesky_serial_bench, serial, cholesky, cholesky)
