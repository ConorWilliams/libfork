#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <bit>
  #include <cstddef>
  #include <memory>
  #include <random>
#else
import std;
#endif

inline constexpr unsigned matmul_test = 64;
inline constexpr unsigned matmul_base = 1024;

inline constexpr unsigned strassen_test = 64;
inline constexpr unsigned strassen_base = 1024;

inline constexpr unsigned matmul_basecase = 32;

static_assert(std::has_single_bit(matmul_test));
static_assert(std::has_single_bit(matmul_base));

struct matmul_args {
  std::unique_ptr<float[]> A;
  std::unique_ptr<float[]> B;
  std::unique_ptr<float[]> C;
  std::unique_ptr<float[]> ref;
  unsigned n;
};

inline auto matmul_init(unsigned n, std::uint64_t seed = 0xC0FFEE) -> matmul_args {

  matmul_args args{
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      n,
  };

  std::mt19937_64 rng{seed};
  std::uniform_real_distribution<float> dist{0, 1};

  for (std::size_t i = 0; i < static_cast<std::size_t>(n) * n; ++i) {
    args.A[i] = dist(rng);
    args.B[i] = dist(rng);
    args.C[i] = 0;
    args.ref[i] = 0;
  }

  return args;
}

inline void matmul_zero(float *C, unsigned n) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(n) * n; ++i) {
    C[i] = 0;
  }
}

inline auto matmul_max_relative_error(float const *A, float const *B, unsigned n) -> float {
  float error = 0;
  for (std::size_t i = 0; i < static_cast<std::size_t>(n) * n; ++i) {
    float diff = (A[i] - B[i]) / A[i];
    if (diff < 0) {
      diff = -diff;
    }
    if (diff > error) {
      error = diff;
    }
  }
  return error;
}

inline void matmul_iter(float const *A, float const *B, float *C, unsigned n) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned k = 0; k < n; ++k) {
      float c = 0;
      for (unsigned j = 0; j < n; ++j) {
        c += A[i * n + j] * B[j * n + k];
      }
      C[i * n + k] = c;
    }
  }
}

template <bool Add>
inline void matmul_basecase_multiply(float const *A, float const *B, float *R, unsigned n, unsigned s) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      float sum = 0;
      for (unsigned k = 0; k < n; ++k) {
        sum += A[i * s + k] * B[k * s + j];
      }
      if constexpr (Add) {
        R[i * s + j] += sum;
      } else {
        R[i * s + j] = sum;
      }
    }
  }
}
