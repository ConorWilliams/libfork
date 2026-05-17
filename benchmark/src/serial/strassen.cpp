#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "matmul.hpp"
#include "strassen.hpp"

import std;

namespace {

void strassen(float const *A, unsigned sa, float const *B, unsigned sb, float *C, unsigned sc, unsigned n) {

  if (n <= strassen_cutoff) {
    matmul_basecase_multiply<false>(A, sa, B, sb, C, sc, n);
    return;
  }

  unsigned m = n / 2;

  float const *A11 = strassen_block(A, sa, m, 0, 0);
  float const *A12 = strassen_block(A, sa, m, 0, 1);
  float const *A21 = strassen_block(A, sa, m, 1, 0);
  float const *A22 = strassen_block(A, sa, m, 1, 1);
  float const *B11 = strassen_block(B, sb, m, 0, 0);
  float const *B12 = strassen_block(B, sb, m, 0, 1);
  float const *B21 = strassen_block(B, sb, m, 1, 0);
  float const *B22 = strassen_block(B, sb, m, 1, 1);
  float *C11 = strassen_block(C, sc, m, 0, 0);
  float *C12 = strassen_block(C, sc, m, 0, 1);
  float *C21 = strassen_block(C, sc, m, 1, 0);
  float *C22 = strassen_block(C, sc, m, 1, 1);

  std::vector<float> buf(strassen_scratch_size(m));
  auto blocks = strassen_scratch_blocks(buf.data(), m);

  strassen_prepare(A11, A12, A21, A22, sa, B11, B12, B21, B22, sb, blocks, m);

  strassen(blocks.S1, m, blocks.S2, m, blocks.M1, m, m);
  strassen(blocks.S3, m, B11, sb, blocks.M2, m, m);
  strassen(A11, sa, blocks.S4, m, blocks.M3, m, m);
  strassen(A22, sa, blocks.S5, m, blocks.M4, m, m);
  strassen(blocks.S6, m, B22, sb, blocks.M5, m, m);
  strassen(blocks.S7, m, blocks.S8, m, blocks.M6, m, m);
  strassen(blocks.S9, m, blocks.S10, m, blocks.M7, m, m);

  strassen_combine(C11, C12, C21, C22, sc, blocks, m);
}

template <typename = void>
void strassen_serial(benchmark::State &state) {
  run_matmul(state, 1e-3f, [](float const *A, float const *B, float *C, unsigned n) {
    strassen(A, n, B, n, C, n, n);
  });
}

} // namespace

BENCH_ALL(strassen_serial, serial, strassen, strassen)
