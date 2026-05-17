#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "transpose.hpp"

import std;

namespace {

void transpose_block_swap(float *A, float *B, unsigned n, unsigned s);

void transpose(float *A, unsigned n, unsigned s) {
  if (n <= transpose_cutoff) {
    transpose_basecase(A, n, s);
    return;
  }

  unsigned const m = n / 2;
  unsigned const o00 = 0;
  unsigned const o01 = m;
  unsigned const o10 = m * s;
  unsigned const o11 = m * s + m;

  transpose(A + o00, m, s);
  transpose(A + o01, m, s);
  transpose(A + o10, m, s);
  transpose(A + o11, m, s);

  transpose_block_swap(A + o01, A + o10, m, s);
}

void transpose_block_swap(float *A, float *B, unsigned n, unsigned s) {
  if (n <= transpose_cutoff) {
    transpose_block_swap_basecase(A, B, n, s);
    return;
  }

  unsigned const m = n / 2;
  unsigned const o00 = 0;
  unsigned const o01 = m;
  unsigned const o10 = m * s;
  unsigned const o11 = m * s + m;

  transpose_block_swap(A + o00, B + o00, m, s);
  transpose_block_swap(A + o01, B + o01, m, s);
  transpose_block_swap(A + o10, B + o10, m, s);
  transpose_block_swap(A + o11, B + o11, m, s);
}

template <typename = void>
void transpose_serial(benchmark::State &state) {
  run_transpose(state, [](float *A, unsigned n) {
    transpose(A, n, n);
  });
}

} // namespace

BENCH_ALL(transpose_serial, serial, transpose, transpose)
