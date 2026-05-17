#pragma once

#include "matmul.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
#else
import std;
#endif

inline constexpr unsigned strassen_test = 64;
inline constexpr unsigned strassen_base = 1024;

inline constexpr unsigned strassen_cutoff = 32;
inline constexpr unsigned strassen_scratch_block_count = 17;

template <typename T>
inline auto strassen_block(T *p, unsigned s, unsigned m, unsigned i, unsigned j) -> T * {
  return p + i * m * s + j * m;
}

inline void strassen_mat_add(
    float const *A, unsigned sa, float const *B, unsigned sb, float *Out, unsigned so, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      Out[i * so + j] = A[i * sa + j] + B[i * sb + j];
    }
  }
}

inline void strassen_mat_sub(
    float const *A, unsigned sa, float const *B, unsigned sb, float *Out, unsigned so, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      Out[i * so + j] = A[i * sa + j] - B[i * sb + j];
    }
  }
}

struct strassen_blocks {
  float *S1;
  float *S2;
  float *S3;
  float *S4;
  float *S5;
  float *S6;
  float *S7;
  float *S8;
  float *S9;
  float *S10;
  float *M1;
  float *M2;
  float *M3;
  float *M4;
  float *M5;
  float *M6;
  float *M7;
};

inline auto strassen_scratch_size(unsigned m) -> std::size_t {
  return static_cast<std::size_t>(m) * m * strassen_scratch_block_count;
}

inline auto strassen_scratch_blocks(float *data, unsigned m) -> strassen_blocks {
  auto const block_size = static_cast<std::size_t>(m) * m;
  return {
      .S1 = data,
      .S2 = data + block_size,
      .S3 = data + 2 * block_size,
      .S4 = data + 3 * block_size,
      .S5 = data + 4 * block_size,
      .S6 = data + 5 * block_size,
      .S7 = data + 6 * block_size,
      .S8 = data + 7 * block_size,
      .S9 = data + 8 * block_size,
      .S10 = data + 9 * block_size,
      .M1 = data + 10 * block_size,
      .M2 = data + 11 * block_size,
      .M3 = data + 12 * block_size,
      .M4 = data + 13 * block_size,
      .M5 = data + 14 * block_size,
      .M6 = data + 15 * block_size,
      .M7 = data + 16 * block_size,
  };
}

inline void strassen_prepare(float const *A11,
                             float const *A12,
                             float const *A21,
                             float const *A22,
                             unsigned sa,
                             float const *B11,
                             float const *B12,
                             float const *B21,
                             float const *B22,
                             unsigned sb,
                             strassen_blocks blocks,
                             unsigned m) {

  strassen_mat_add(A11, sa, A22, sa, blocks.S1, m, m);  // S1 = A11 + A22
  strassen_mat_add(B11, sb, B22, sb, blocks.S2, m, m);  // S2 = B11 + B22
  strassen_mat_add(A21, sa, A22, sa, blocks.S3, m, m);  // S3 = A21 + A22
  strassen_mat_sub(B12, sb, B22, sb, blocks.S4, m, m);  // S4 = B12 - B22
  strassen_mat_sub(B21, sb, B11, sb, blocks.S5, m, m);  // S5 = B21 - B11
  strassen_mat_add(A11, sa, A12, sa, blocks.S6, m, m);  // S6 = A11 + A12
  strassen_mat_sub(A21, sa, A11, sa, blocks.S7, m, m);  // S7 = A21 - A11
  strassen_mat_add(B11, sb, B12, sb, blocks.S8, m, m);  // S8 = B11 + B12
  strassen_mat_sub(A12, sa, A22, sa, blocks.S9, m, m);  // S9 = A12 - A22
  strassen_mat_add(B21, sb, B22, sb, blocks.S10, m, m); // S10 = B21 + B22
}

inline void strassen_combine(
    float *C11, float *C12, float *C21, float *C22, unsigned sc, strassen_blocks blocks, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      std::size_t k = static_cast<std::size_t>(i) * m + j;
      C11[i * sc + j] = blocks.M1[k] + blocks.M4[k] - blocks.M5[k] + blocks.M7[k];
      C12[i * sc + j] = blocks.M3[k] + blocks.M5[k];
      C21[i * sc + j] = blocks.M2[k] + blocks.M4[k];
      C22[i * sc + j] = blocks.M1[k] - blocks.M2[k] + blocks.M3[k] + blocks.M6[k];
    }
  }
}
