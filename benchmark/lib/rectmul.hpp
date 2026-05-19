#pragma once

#include "matrix.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <vector>
#else
import std;
#endif

inline constexpr long rectmul_test = 512;
inline constexpr long rectmul_base = 4096;
inline constexpr long rectmul_block_edge = 16;
inline constexpr long rectmul_block_size = rectmul_block_edge * rectmul_block_edge;
inline constexpr long rectmul_inner_divisor = 3;
inline constexpr long rectmul_cols_divisor = 5;

using rectmul_block = std::array<double, rectmul_block_size>;

struct rectmul_problem {
  std::vector<rectmul_block> A;
  std::vector<rectmul_block> B;
  std::vector<rectmul_block> R;
  matrix_middle_value_t<double> middle;
  long x;
  long y;
  long z;
};

inline auto rectmul_block_at(rectmul_block *blocks, long stride, long row, long col) -> rectmul_block * {
  return matrix_block_at(blocks, stride, row, col);
}

inline auto
rectmul_block_at(rectmul_block const *blocks, long stride, long row, long col) -> rectmul_block const * {
  return matrix_block_at(blocks, stride, row, col);
}

inline void rectmul_init_matrix(rectmul_block *R, long x, long y, long stride, double value) {
  matrix_fill_blocks(R, x, y, stride, value);
}

inline void rectmul_init_lhs_matrix(rectmul_block *R, long x, long y, long stride) {
  matrix_fill_blocks_with(R, x, y, stride, rectmul_block_edge, matrix_lhs_value);
}

inline void rectmul_init_rhs_matrix(rectmul_block *R, long x, long y, long stride) {
  matrix_fill_blocks_with(R, x, y, stride, rectmul_block_edge, matrix_rhs_value);
}

inline auto rectmul_check_matrix(rectmul_block const *R,
                                 long x,
                                 long y,
                                 long stride,
                                 long inner,
                                 matrix_middle_value_t<double> const &middle,
                                 double max_relative_error) -> bool {
  return check_matrix_multiply_blocks(R, x, y, stride, rectmul_block_edge, inner, middle, max_relative_error);
}

inline auto rectmul_adversarial_extent(long block_count, long divisor) -> long {
  long extent = std::max(1L, block_count / divisor);
  if (extent % 2 == 0) {
    ++extent;
  }
  return extent;
}

inline auto rectmul_make(long n) -> rectmul_problem {
  long const block_count = n / rectmul_block_edge;
  long const x = block_count;
  long const y = rectmul_adversarial_extent(block_count, rectmul_inner_divisor);
  long const z = rectmul_adversarial_extent(block_count, rectmul_cols_divisor);
  rectmul_problem problem{
      .A = std::vector<rectmul_block>(static_cast<std::size_t>(x * y)),
      .B = std::vector<rectmul_block>(static_cast<std::size_t>(y * z)),
      .R = std::vector<rectmul_block>(static_cast<std::size_t>(x * z)),
      .middle = matrix_multiply_middle<double>(static_cast<unsigned>(y * rectmul_block_edge)),
      .x = x,
      .y = y,
      .z = z,
  };
  rectmul_init_lhs_matrix(problem.A.data(), x, y, y);
  rectmul_init_rhs_matrix(problem.B.data(), y, z, z);
  return problem;
}

inline void rectmul_multiply_block(rectmul_block const &A, rectmul_block const &B, rectmul_block &R) {
  for (long i = 0; i < rectmul_block_edge; ++i) {
    for (long j = 0; j < rectmul_block_edge; ++j) {
      double sum = 0.0;
      for (long k = 0; k < rectmul_block_edge; ++k) {
        sum += A[static_cast<std::size_t>(i * rectmul_block_edge + k)] *
               B[static_cast<std::size_t>(k * rectmul_block_edge + j)];
      }
      R[static_cast<std::size_t>(i * rectmul_block_edge + j)] = sum;
    }
  }
}

inline void rectmul_mult_add_block(rectmul_block const &A, rectmul_block const &B, rectmul_block &R) {
  for (long i = 0; i < rectmul_block_edge; ++i) {
    for (long j = 0; j < rectmul_block_edge; ++j) {
      double sum = 0.0;
      for (long k = 0; k < rectmul_block_edge; ++k) {
        sum += A[static_cast<std::size_t>(i * rectmul_block_edge + k)] *
               B[static_cast<std::size_t>(k * rectmul_block_edge + j)];
      }
      R[static_cast<std::size_t>(i * rectmul_block_edge + j)] += sum;
    }
  }
}

inline void rectmul_add_matrix(rectmul_block const *T, long ot, rectmul_block *R, long oR, long x, long y) {
  if (x + y == 2) {
    for (long i = 0; i < rectmul_block_size; ++i) {
      (*R)[static_cast<std::size_t>(i)] += (*T)[static_cast<std::size_t>(i)];
    }
    return;
  }

  if (x > y) {
    rectmul_add_matrix(T, ot, R, oR, x / 2, y);
    rectmul_add_matrix(T + (x / 2) * ot, ot, R + (x / 2) * oR, oR, (x + 1) / 2, y);
  } else {
    rectmul_add_matrix(T, ot, R, oR, x, y / 2);
    rectmul_add_matrix(T + (y / 2), ot, R + (y / 2), oR, x, (y + 1) / 2);
  }
}

template <typename Fn>
void run_rectmul(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<long>(state.range(0));
  auto problem = rectmul_make(n);

  state.counters["n"] = static_cast<double>(n);
  state.counters["x"] = static_cast<double>(problem.x * rectmul_block_edge);
  state.counters["y"] = static_cast<double>(problem.y * rectmul_block_edge);
  state.counters["z"] = static_cast<double>(problem.z * rectmul_block_edge);
  state.counters["block"] = static_cast<double>(rectmul_block_edge);

  lf_bench::bench(state, threads, true, [&]() -> bool {
    state.PauseTiming();
    rectmul_init_matrix(problem.R.data(), problem.x, problem.z, problem.z, 0.0);
    state.ResumeTiming();

    std::invoke(fn,
                problem.A.data(),
                problem.y,
                problem.B.data(),
                problem.z,
                problem.x,
                problem.y,
                problem.z,
                problem.R.data(),
                problem.z,
                0);

    benchmark::DoNotOptimize(problem.R.data());
    return rectmul_check_matrix(
        problem.R.data(), problem.x, problem.z, problem.z, problem.y, problem.middle, 1e-10);
  });
}

template <typename Fn>
void run_rectmul(benchmark::State &state, Fn fn) {
  run_rectmul(state, lf_bench::no_threads, fn);
}
