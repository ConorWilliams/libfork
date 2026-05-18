#pragma once

#include "matmul.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
  #include <vector>
#else
import std;
#endif

inline constexpr unsigned strassen_test = 64;
inline constexpr unsigned strassen_base = 1024;

inline constexpr unsigned strassen_cutoff = 32;
inline constexpr unsigned strassen_scratch_block_count = 17;
inline constexpr unsigned strassen_winograd_divide_cutoff = 128;
inline constexpr unsigned strassen_winograd_naive_cutoff = 16;
inline constexpr unsigned strassen_winograd_scratch_block_count = 11;
inline constexpr unsigned strassen_winograd_loop_cutoff = 1;

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

inline void strassen_combine_00(float *C, unsigned sc, strassen_blocks blocks, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      std::size_t k = static_cast<std::size_t>(i) * m + j;
      C[i * sc + j] = blocks.M1[k] + blocks.M4[k] - blocks.M5[k] + blocks.M7[k];
    }
  }
}

inline void strassen_combine_01(float *C, unsigned sc, strassen_blocks blocks, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      std::size_t k = static_cast<std::size_t>(i) * m + j;
      C[i * sc + j] = blocks.M3[k] + blocks.M5[k];
    }
  }
}

inline void strassen_combine_10(float *C, unsigned sc, strassen_blocks blocks, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      std::size_t k = static_cast<std::size_t>(i) * m + j;
      C[i * sc + j] = blocks.M2[k] + blocks.M4[k];
    }
  }
}

inline void strassen_combine_11(float *C, unsigned sc, strassen_blocks blocks, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      std::size_t k = static_cast<std::size_t>(i) * m + j;
      C[i * sc + j] = blocks.M1[k] - blocks.M2[k] + blocks.M3[k] + blocks.M6[k];
    }
  }
}

struct strassen_winograd_blocks {
  float *S1;
  float *S2;
  float *S3;
  float *S4;
  float *S5;
  float *S6;
  float *S7;
  float *S8;
  float *M2;
  float *M5;
  float *T1;
};

inline auto strassen_winograd_scratch_size(unsigned m) -> std::size_t {
  return static_cast<std::size_t>(m) * m * strassen_winograd_scratch_block_count;
}

inline auto strassen_winograd_scratch_blocks(float *data, unsigned m) -> strassen_winograd_blocks {
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
      .M2 = data + 8 * block_size,
      .M5 = data + 9 * block_size,
      .T1 = data + 10 * block_size,
  };
}

inline void strassen_winograd_prepare_range(float const *A11,
                                            unsigned sa,
                                            float const *A12,
                                            float const *A21,
                                            float const *A22,
                                            float const *B11,
                                            unsigned sb,
                                            float const *B12,
                                            float const *B21,
                                            float const *B22,
                                            strassen_winograd_blocks blocks,
                                            unsigned m,
                                            unsigned row_begin,
                                            unsigned row_end) {
  for (unsigned i = row_begin; i < row_end; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      auto idx = static_cast<std::size_t>(i) * m + j;
      auto a_idx = static_cast<std::size_t>(i) * sa + j;
      auto b_idx = static_cast<std::size_t>(i) * sb + j;

      blocks.S1[idx] = A21[a_idx] + A22[a_idx];
      blocks.S2[idx] = blocks.S1[idx] - A11[a_idx];
      blocks.S4[idx] = A12[a_idx] - blocks.S2[idx];
      blocks.S3[idx] = A11[a_idx] - A21[a_idx];

      blocks.S5[idx] = B12[b_idx] - B11[b_idx];
      blocks.S6[idx] = B22[b_idx] - blocks.S5[idx];
      blocks.S8[idx] = blocks.S6[idx] - B21[b_idx];
      blocks.S7[idx] = B22[b_idx] - B12[b_idx];
    }
  }
}

inline void strassen_winograd_combine_range(float *C11,
                                            float *C12,
                                            float *C21,
                                            float *C22,
                                            unsigned sc,
                                            strassen_winograd_blocks blocks,
                                            unsigned m,
                                            unsigned row_begin,
                                            unsigned row_end) {
  for (unsigned i = row_begin; i < row_end; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      auto idx = static_cast<std::size_t>(i) * m + j;
      auto c_idx = static_cast<std::size_t>(i) * sc + j;
      float t1 = blocks.T1[idx] + blocks.M2[idx];
      float t2 = C22[c_idx] + t1;

      C11[c_idx] += blocks.M2[idx];
      C12[c_idx] += blocks.M5[idx] + t1;
      C22[c_idx] = blocks.M5[idx] + t2;
      C21[c_idx] = -C21[c_idx] + t2;
    }
  }
}

inline void strassen_winograd_dac(float *C,
                                  unsigned sc,
                                  float const *A,
                                  unsigned sa,
                                  float const *B,
                                  unsigned sb,
                                  unsigned n,
                                  bool additive) {
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

  auto multiply =
      [&](float *out, unsigned so, float const *lhs, unsigned sl, float const *rhs, unsigned sr, bool add) {
        if (m > strassen_winograd_naive_cutoff) {
          strassen_winograd_dac(out, so, lhs, sl, rhs, sr, m, add);
        } else if (add) {
          matmul_basecase_multiply<true>(lhs, sl, rhs, sr, out, so, m);
        } else {
          matmul_basecase_multiply<false>(lhs, sl, rhs, sr, out, so, m);
        }
      };

  multiply(C11, sc, A11, sa, B11, sb, additive);
  multiply(C12, sc, A11, sa, B12, sb, additive);
  multiply(C22, sc, A21, sa, B12, sb, additive);
  multiply(C21, sc, A21, sa, B11, sb, additive);

  multiply(C11, sc, A12, sa, B21, sb, true);
  multiply(C12, sc, A12, sa, B22, sb, true);
  multiply(C22, sc, A22, sa, B22, sb, true);
  multiply(C21, sc, A22, sa, B21, sb, true);
}

inline void strassen_winograd(
    float const *A, unsigned sa, float const *B, unsigned sb, float *C, unsigned sc, unsigned n) {
  if (n <= strassen_winograd_divide_cutoff) {
    strassen_winograd_dac(C, sc, A, sa, B, sb, n, false);
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

  std::vector<float> buf(strassen_winograd_scratch_size(m));
  auto blocks = strassen_winograd_scratch_blocks(buf.data(), m);

  strassen_winograd_prepare_range(A11, sa, A12, A21, A22, B11, sb, B12, B21, B22, blocks, m, 0, m);

  strassen_winograd(A11, sa, B11, sb, blocks.M2, m, m);
  strassen_winograd(blocks.S1, m, blocks.S5, m, blocks.M5, m, m);
  strassen_winograd(blocks.S2, m, blocks.S6, m, blocks.T1, m, m);
  strassen_winograd(blocks.S3, m, blocks.S7, m, C22, sc, m);
  strassen_winograd(A12, sa, B21, sb, C11, sc, m);
  strassen_winograd(blocks.S4, m, B22, sb, C12, sc, m);
  strassen_winograd(A22, sa, blocks.S8, m, C21, sc, m);

  strassen_winograd_combine_range(C11, C12, C21, C22, sc, blocks, m, 0, m);
}
