#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "matrix.hpp"
#include "strassen.hpp"

import std;

namespace {

void strassen(float const *A, unsigned sa, float const *B, unsigned sb, float *C, unsigned sc, unsigned n) {

  if (n <= strassen_cutoff) {
    matrix_multiply_basecase<false>(A, sa, B, sb, C, sc, n);
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

  strassen_mat_add(A11, sa, A22, sa, blocks.S1, m, m);
  strassen_mat_add(B11, sb, B22, sb, blocks.S2, m, m);
  strassen_mat_add(A21, sa, A22, sa, blocks.S3, m, m);
  strassen_mat_sub(B12, sb, B22, sb, blocks.S4, m, m);
  strassen_mat_sub(B21, sb, B11, sb, blocks.S5, m, m);
  strassen_mat_add(A11, sa, A12, sa, blocks.S6, m, m);
  strassen_mat_sub(A21, sa, A11, sa, blocks.S7, m, m);
  strassen_mat_add(B11, sb, B12, sb, blocks.S8, m, m);
  strassen_mat_sub(A12, sa, A22, sa, blocks.S9, m, m);
  strassen_mat_add(B21, sb, B22, sb, blocks.S10, m, m);

  strassen(blocks.S1, m, blocks.S2, m, blocks.M1, m, m);
  strassen(blocks.S3, m, B11, sb, blocks.M2, m, m);
  strassen(A11, sa, blocks.S4, m, blocks.M3, m, m);
  strassen(A22, sa, blocks.S5, m, blocks.M4, m, m);
  strassen(blocks.S6, m, B22, sb, blocks.M5, m, m);
  strassen(blocks.S7, m, blocks.S8, m, blocks.M6, m, m);
  strassen(blocks.S9, m, blocks.S10, m, blocks.M7, m, m);

  strassen_combine_00(C11, sc, blocks, m);
  strassen_combine_01(C12, sc, blocks, m);
  strassen_combine_10(C21, sc, blocks, m);
  strassen_combine_11(C22, sc, blocks, m);
}

template <typename = void>
void strassen_serial(benchmark::State &state) {
  run_matrix_multiply(state, 2e-3f, [](float const *A, float const *B, float *C, unsigned n) {
    strassen(A, n, B, n, C, n, n);
  });
}

} // namespace

BENCH_ALL(strassen_serial, serial, strassen, strassen)
