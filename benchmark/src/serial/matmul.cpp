#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "matmul.hpp"

import std;

namespace {

template <bool Add>
void matmul_dc(float const *A, float const *B, float *R, unsigned n, unsigned s) {
  if (n <= matmul_cutoff) {
    matrix_multiply_basecase<Add>(A, B, R, n, s);
    return;
  }

  unsigned m = n / 2;

  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  matmul_dc<Add>(A + o00, B + o00, R + o00, m, s);
  matmul_dc<Add>(A + o10, B + o00, R + o10, m, s);
  matmul_dc<Add>(A + o00, B + o01, R + o01, m, s);
  matmul_dc<Add>(A + o10, B + o01, R + o11, m, s);

  matmul_dc<true>(A + o01, B + o10, R + o00, m, s);
  matmul_dc<true>(A + o01, B + o11, R + o01, m, s);
  matmul_dc<true>(A + o11, B + o10, R + o10, m, s);
  matmul_dc<true>(A + o11, B + o11, R + o11, m, s);
}

template <typename = void>
void matmul_serial(benchmark::State &state) {
  run_matmul(state, 1e-5f, [](float const *A, float const *B, float *C, unsigned n) {
    matmul_dc<false>(A, B, C, n, n);
  });
}

} // namespace

BENCH_ALL(matmul_serial, serial, matmul, matmul)
