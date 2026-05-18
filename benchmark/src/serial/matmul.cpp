#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "matmul.hpp"

import std;

namespace {

void matmul_dc(float const *A, float const *B, float *R, unsigned n, unsigned s);

void matmul_compute00(float const *A, float const *B, float *R, unsigned m, unsigned s) {
  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;

  matmul_dc(A + o00, B + o00, R + o00, m, s);
  matmul_dc(A + o01, B + o10, R + o00, m, s);
}

void matmul_compute10(float const *A, float const *B, float *R, unsigned m, unsigned s) {
  unsigned o00 = 0;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  matmul_dc(A + o10, B + o00, R + o10, m, s);
  matmul_dc(A + o11, B + o10, R + o10, m, s);
}

void matmul_compute01(float const *A, float const *B, float *R, unsigned m, unsigned s) {
  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o11 = m * s + m;

  matmul_dc(A + o00, B + o01, R + o01, m, s);
  matmul_dc(A + o01, B + o11, R + o01, m, s);
}

void matmul_compute11(float const *A, float const *B, float *R, unsigned m, unsigned s) {
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  matmul_dc(A + o10, B + o01, R + o11, m, s);
  matmul_dc(A + o11, B + o11, R + o11, m, s);
}

void matmul_dc(float const *A, float const *B, float *R, unsigned n, unsigned s) {
  if (n <= matmul_cutoff) {
    matmul_basecase_multiply<true>(A, B, R, n, s);
    return;
  }

  unsigned m = n / 2;

  matmul_compute00(A, B, R, m, s);
  matmul_compute10(A, B, R, m, s);
  matmul_compute01(A, B, R, m, s);
  matmul_compute11(A, B, R, m, s);
}

template <typename = void>
void matmul_serial(benchmark::State &state) {
  run_matmul(state, 1e-5f, [](float const *A, float const *B, float *C, unsigned n) {
    matmul_dc(A, B, C, n, n);
  });
}

} // namespace

BENCH_ALL(matmul_serial, serial, matmul, matmul)
