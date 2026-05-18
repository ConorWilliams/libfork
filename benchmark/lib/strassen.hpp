#pragma once

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
