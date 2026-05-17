#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <bit>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <memory>
#else
import std;
#endif

inline constexpr unsigned matmul_test = 64;
inline constexpr unsigned matmul_base = 1024;

inline constexpr unsigned strassen_test = 64;
inline constexpr unsigned strassen_base = 1024;

inline constexpr unsigned matmul_basecase = 32;
inline constexpr unsigned matmul_check_rank = 3;

static_assert(std::has_single_bit(matmul_test));
static_assert(std::has_single_bit(matmul_base));

struct matmul_args {
  std::unique_ptr<float[]> A;
  std::unique_ptr<float[]> B;
  std::unique_ptr<float[]> C;
  std::array<float, matmul_check_rank * matmul_check_rank> middle;
  unsigned n;
};

inline auto matmul_factor(unsigned index, unsigned rank, unsigned salt) -> float {
  return static_cast<float>(((index + 1U) * (rank + salt) + 7U * rank + salt) % 15U + 1U) / 16.0F;
}

inline auto matmul_lhs_row(unsigned i, unsigned r) -> float { return matmul_factor(i, r, 3); }

inline auto matmul_lhs_col(unsigned j, unsigned r) -> float { return matmul_factor(j, r, 5); }

inline auto matmul_rhs_row(unsigned j, unsigned r) -> float { return matmul_factor(j, r, 7); }

inline auto matmul_rhs_col(unsigned k, unsigned r) -> float { return matmul_factor(k, r, 11); }

inline auto matmul_lhs_diag(unsigned i) -> float { return 1.0F + matmul_factor(i, 0, 13); }

inline auto matmul_rhs_diag(unsigned i) -> float { return 1.0F + matmul_factor(i, 0, 14); }

// Build dense matrices A = D_a + U V^T and B = D_b + X Y^T. Their product can
// be checked from V^T X without doing another matrix multiplication.
inline auto matmul_lhs_value(unsigned i, unsigned j) -> float {
  float value = 0;
  if (i == j) {
    value += matmul_lhs_diag(i);
  }
  for (unsigned r = 0; r < matmul_check_rank; ++r) {
    value += matmul_lhs_row(i, r) * matmul_lhs_col(j, r);
  }
  return value;
}

inline auto matmul_rhs_value(unsigned j, unsigned k) -> float {
  float value = 0;
  if (j == k) {
    value += matmul_rhs_diag(j);
  }
  for (unsigned r = 0; r < matmul_check_rank; ++r) {
    value += matmul_rhs_row(j, r) * matmul_rhs_col(k, r);
  }
  return value;
}

inline auto matmul_middle(unsigned n) -> std::array<float, matmul_check_rank * matmul_check_rank> {
  std::array<float, matmul_check_rank * matmul_check_rank> middle{};
  for (unsigned j = 0; j < n; ++j) {
    for (unsigned r = 0; r < matmul_check_rank; ++r) {
      for (unsigned s = 0; s < matmul_check_rank; ++s) {
        middle[r * matmul_check_rank + s] += matmul_lhs_col(j, r) * matmul_rhs_row(j, s);
      }
    }
  }
  return middle;
}

inline auto matmul_expected_value(std::array<float, matmul_check_rank * matmul_check_rank> const &middle,
                                  unsigned i,
                                  unsigned k) -> float {
  float value = i == k ? matmul_lhs_diag(i) * matmul_rhs_diag(i) : 0;

  for (unsigned s = 0; s < matmul_check_rank; ++s) {
    value += matmul_lhs_diag(i) * matmul_rhs_row(i, s) * matmul_rhs_col(k, s);
  }
  for (unsigned r = 0; r < matmul_check_rank; ++r) {
    value += matmul_lhs_row(i, r) * matmul_lhs_col(k, r) * matmul_rhs_diag(k);
  }
  for (unsigned r = 0; r < matmul_check_rank; ++r) {
    for (unsigned s = 0; s < matmul_check_rank; ++s) {
      value += matmul_lhs_row(i, r) * middle[r * matmul_check_rank + s] * matmul_rhs_col(k, s);
    }
  }
  return value;
}

inline auto matmul_init(unsigned n) -> matmul_args {

  matmul_args args{
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      matmul_middle(n),
      n,
  };

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      args.A[static_cast<std::size_t>(i) * n + j] = matmul_lhs_value(i, j);
      args.B[static_cast<std::size_t>(i) * n + j] = matmul_rhs_value(i, j);
      args.C[static_cast<std::size_t>(i) * n + j] = 0;
    }
  }

  return args;
}

inline void matmul_zero(float *C, unsigned n) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(n) * n; ++i) {
    C[i] = 0;
  }
}

inline auto matmul_max_relative_error(float const *C,
                                      std::array<float, matmul_check_rank * matmul_check_rank> const &middle,
                                      unsigned n) -> float {
  constexpr float epsilon = 1e-8F;
  float error = 0;
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned k = 0; k < n; ++k) {
      float expect = matmul_expected_value(middle, i, k);
      float actual = C[static_cast<std::size_t>(i) * n + k];
      float diff = std::abs(expect - actual) / std::max(std::abs(expect), epsilon);
      if (diff > error) {
        error = diff;
      }
    }
  }
  return error;
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

inline auto matmul_error_is_acceptable(float err, float max_err) -> bool { return err <= max_err; }

template <typename Fn>
void run_matmul(benchmark::State &state, std::int64_t threads, float max_relative_error, Fn fn) {
  auto n = static_cast<unsigned>(state.range(0));
  state.counters["n"] = n;

  auto args = matmul_init(n);

  lf_bench::bench(state, threads, max_relative_error, matmul_error_is_acceptable, [&]() -> float {
    matmul_zero(args.C.get(), n);
    std::invoke(fn, args.A.get(), args.B.get(), args.C.get(), n);
    benchmark::DoNotOptimize(args.C.get());
    return matmul_max_relative_error(args.C.get(), args.middle, n);
  });
}

template <typename Fn>
void run_matmul(benchmark::State &state, float max_relative_error, Fn fn) {
  run_matmul(state, lf_bench::no_threads, max_relative_error, fn);
}
