#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <memory>
#else
import std;
#endif

inline constexpr unsigned matrix_check_rank = 3;

template <typename Value>
using matrix_middle_value_t = std::array<Value, matrix_check_rank * matrix_check_rank>;

using matrix_buffer_t = std::unique_ptr<float[]>;
using matrix_middle_t = matrix_middle_value_t<float>;

struct matrix_multiply_args {
  matrix_buffer_t A;
  matrix_buffer_t B;
  matrix_buffer_t C;
  matrix_middle_t middle;
  unsigned n;
};

struct matrix_multiply_output {
  float const *C;
  matrix_middle_t const *middle;
  unsigned n;
};

inline auto matrix_factor(unsigned index, unsigned rank, unsigned salt) -> float {
  return static_cast<float>(((index + 1U) * (rank + salt) + 7U * rank + salt) % 15U + 1U) / 16.0F;
}

inline auto matrix_lhs_row(unsigned i, unsigned r) -> float { return matrix_factor(i, r, 3); }

inline auto matrix_lhs_col(unsigned j, unsigned r) -> float { return matrix_factor(j, r, 5); }

inline auto matrix_rhs_row(unsigned j, unsigned r) -> float { return matrix_factor(j, r, 7); }

inline auto matrix_rhs_col(unsigned k, unsigned r) -> float { return matrix_factor(k, r, 11); }

inline auto matrix_lhs_diag(unsigned i) -> float { return 1.0F + matrix_factor(i, 0, 13); }

inline auto matrix_rhs_diag(unsigned i) -> float { return 1.0F + matrix_factor(i, 0, 14); }

// Build dense matrix values A = D_a + U V^T and B = D_b + X Y^T. Their
// product can be checked from V^T X without doing another multiplication.
inline auto matrix_lhs_value(unsigned i, unsigned j) -> float {
  float value = 0;
  if (i == j) {
    value += matrix_lhs_diag(i);
  }
  for (unsigned r = 0; r < matrix_check_rank; ++r) {
    value += matrix_lhs_row(i, r) * matrix_lhs_col(j, r);
  }
  return value;
}

inline auto matrix_rhs_value(unsigned j, unsigned k) -> float {
  float value = 0;
  if (j == k) {
    value += matrix_rhs_diag(j);
  }
  for (unsigned r = 0; r < matrix_check_rank; ++r) {
    value += matrix_rhs_row(j, r) * matrix_rhs_col(k, r);
  }
  return value;
}

template <typename Value = float>
inline auto matrix_multiply_middle(unsigned n) -> matrix_middle_value_t<Value> {
  matrix_middle_value_t<Value> middle{};
  for (unsigned j = 0; j < n; ++j) {
    for (unsigned r = 0; r < matrix_check_rank; ++r) {
      for (unsigned s = 0; s < matrix_check_rank; ++s) {
        middle[r * matrix_check_rank + s] +=
            static_cast<Value>(matrix_lhs_col(j, r)) * static_cast<Value>(matrix_rhs_row(j, s));
      }
    }
  }
  return middle;
}

template <typename Value>
inline auto matrix_multiply_expected_value(matrix_middle_value_t<Value> const &middle,
                                           unsigned i,
                                           unsigned k,
                                           unsigned inner_dimension) -> Value {
  Value value = i == k && i < inner_dimension
                    ? static_cast<Value>(matrix_lhs_diag(i)) * static_cast<Value>(matrix_rhs_diag(i))
                    : Value{};

  if (i < inner_dimension) {
    for (unsigned s = 0; s < matrix_check_rank; ++s) {
      value += static_cast<Value>(matrix_lhs_diag(i)) * static_cast<Value>(matrix_rhs_row(i, s)) *
               static_cast<Value>(matrix_rhs_col(k, s));
    }
  }
  if (k < inner_dimension) {
    for (unsigned r = 0; r < matrix_check_rank; ++r) {
      value += static_cast<Value>(matrix_lhs_row(i, r)) * static_cast<Value>(matrix_lhs_col(k, r)) *
               static_cast<Value>(matrix_rhs_diag(k));
    }
  }
  for (unsigned r = 0; r < matrix_check_rank; ++r) {
    for (unsigned s = 0; s < matrix_check_rank; ++s) {
      value += static_cast<Value>(matrix_lhs_row(i, r)) * middle[r * matrix_check_rank + s] *
               static_cast<Value>(matrix_rhs_col(k, s));
    }
  }
  return value;
}

inline auto matrix_multiply_init(unsigned n) -> matrix_multiply_args {

  matrix_multiply_args args{
      .A = std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      .B = std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      .C = std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      .middle = matrix_multiply_middle(n),
      .n = n,
  };

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      args.A[static_cast<std::size_t>(i) * n + j] = matrix_lhs_value(i, j);
      args.B[static_cast<std::size_t>(i) * n + j] = matrix_rhs_value(i, j);
      args.C[static_cast<std::size_t>(i) * n + j] = 0;
    }
  }

  return args;
}

inline void matrix_zero(float *C, unsigned n) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(n) * n; ++i) {
    C[i] = 0;
  }
}

template <typename Expected, typename Actual>
inline auto matrix_relative_error(Expected expected, Actual actual) -> double {
  constexpr double epsilon = 1e-8;
  double const expected_value = static_cast<double>(expected);
  double const actual_value = static_cast<double>(actual);
  return std::abs(expected_value - actual_value) / std::max(std::abs(expected_value), epsilon);
}

inline auto
matrix_multiply_max_relative_error(float const *C, matrix_middle_t const &middle, unsigned n) -> double {
  double error = 0;

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned k = 0; k < n; ++k) {
      float const expected = matrix_multiply_expected_value(middle, i, k, n);
      float const actual = C[static_cast<std::size_t>(i) * n + k];
      error = std::max(matrix_relative_error(expected, actual), error);
    }
  }

  return error;
}

inline auto check_matrix_multiply(matrix_multiply_output output, float max_err) -> bool {
  return matrix_multiply_max_relative_error(output.C, *output.middle, output.n) <=
         static_cast<double>(max_err);
}

template <bool Add>
inline void matrix_multiply_basecase(
    float const *A, unsigned sa, float const *B, unsigned sb, float *R, unsigned sr, unsigned n) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      float sum = 0;
      for (unsigned k = 0; k < n; ++k) {
        sum += A[i * sa + k] * B[k * sb + j];
      }
      if constexpr (Add) {
        R[i * sr + j] += sum;
      } else {
        R[i * sr + j] = sum;
      }
    }
  }
}

template <bool Add>
inline void matrix_multiply_basecase(float const *A, float const *B, float *R, unsigned n, unsigned s) {
  matrix_multiply_basecase<Add>(A, s, B, s, R, s, n);
}

template <typename Block, typename Size>
inline auto matrix_block_at(Block *blocks, Size stride, Size row, Size col) -> Block * {
  return blocks + static_cast<std::size_t>(row * stride + col);
}

template <typename Block, typename Size>
inline auto matrix_block_at(Block const *blocks, Size stride, Size row, Size col) -> Block const * {
  return blocks + static_cast<std::size_t>(row * stride + col);
}

template <typename Block, typename Size, typename Value>
inline void matrix_fill_blocks(Block *blocks, Size rows, Size cols, Size stride, Value value) {
  for (Size i = 0; i < rows; ++i) {
    for (Size j = 0; j < cols; ++j) {
      matrix_block_at(blocks, stride, i, j)->fill(value);
    }
  }
}

template <typename Block, typename Size, typename Fn>
inline void
matrix_fill_blocks_with(Block *blocks, Size rows, Size cols, Size stride, Size block_edge, Fn value) {
  for (Size block_i = 0; block_i < rows; ++block_i) {
    for (Size block_j = 0; block_j < cols; ++block_j) {
      auto &block = *matrix_block_at(blocks, stride, block_i, block_j);
      for (Size i = 0; i < block_edge; ++i) {
        for (Size j = 0; j < block_edge; ++j) {
          auto const row = static_cast<unsigned>(block_i * block_edge + i);
          auto const col = static_cast<unsigned>(block_j * block_edge + j);
          block[static_cast<std::size_t>(i * block_edge + j)] =
              static_cast<typename Block::value_type>(std::invoke(value, row, col));
        }
      }
    }
  }
}

template <typename Block, typename Size, typename Value>
inline auto matrix_check_blocks(Block const *blocks, Size rows, Size cols, Size stride, Value value) -> bool {
  for (Size i = 0; i < rows; ++i) {
    for (Size j = 0; j < cols; ++j) {
      auto const &block = *matrix_block_at(blocks, stride, i, j);
      for (auto actual : block) {
        if (std::abs(actual - value) > Value{}) {
          return false;
        }
      }
    }
  }
  return true;
}

template <typename Block, typename Size, typename Middle>
inline auto matrix_multiply_blocks_max_relative_error(Block const *blocks,
                                                      Size rows,
                                                      Size cols,
                                                      Size stride,
                                                      Size block_edge,
                                                      Size inner_blocks,
                                                      Middle const &middle) -> double {
  double error = 0;
  auto const inner_dimension = static_cast<unsigned>(inner_blocks * block_edge);

  for (Size block_i = 0; block_i < rows; ++block_i) {
    for (Size block_j = 0; block_j < cols; ++block_j) {
      auto const &block = *matrix_block_at(blocks, stride, block_i, block_j);
      for (Size i = 0; i < block_edge; ++i) {
        for (Size j = 0; j < block_edge; ++j) {
          auto const row = static_cast<unsigned>(block_i * block_edge + i);
          auto const col = static_cast<unsigned>(block_j * block_edge + j);
          auto const expected = matrix_multiply_expected_value(middle, row, col, inner_dimension);
          auto const actual = block[static_cast<std::size_t>(i * block_edge + j)];
          error = std::max(matrix_relative_error(expected, actual), error);
        }
      }
    }
  }

  return error;
}

template <typename Block, typename Size, typename Middle>
inline auto check_matrix_multiply_blocks(Block const *blocks,
                                         Size rows,
                                         Size cols,
                                         Size stride,
                                         Size block_edge,
                                         Size inner_blocks,
                                         Middle const &middle,
                                         double max_error) -> bool {
  return matrix_multiply_blocks_max_relative_error(
             blocks, rows, cols, stride, block_edge, inner_blocks, middle) <= max_error;
}

template <typename Fn>
void run_matrix_multiply(benchmark::State &state, std::int64_t threads, float max_relative_error, Fn fn) {
  auto n = static_cast<unsigned>(state.range(0));
  state.counters["n"] = n;

  auto args = matrix_multiply_init(n);

  lf_bench::bench(state, threads, max_relative_error, check_matrix_multiply, [&]() -> matrix_multiply_output {
    state.PauseTiming();
    matrix_zero(args.C.get(), n);
    state.ResumeTiming();

    std::invoke(fn, args.A.get(), args.B.get(), args.C.get(), n);

    return {.C = args.C.get(), .middle = &args.middle, .n = n};
  });
}

template <typename Fn>
void run_matrix_multiply(benchmark::State &state, float max_relative_error, Fn fn) {
  run_matrix_multiply(state, lf_bench::no_threads, max_relative_error, fn);
}
