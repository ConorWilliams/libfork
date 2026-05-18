#pragma once

#include "bench.hpp"

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

using rectmul_block = std::array<double, rectmul_block_size>;

struct rectmul_problem {
  std::vector<rectmul_block> A;
  std::vector<rectmul_block> B;
  std::vector<rectmul_block> R;
  long x;
  long y;
  long z;
};

inline auto rectmul_block_at(rectmul_block *blocks, long stride, long row, long col) -> rectmul_block * {
  return blocks + static_cast<std::size_t>(row * stride + col);
}

inline auto rectmul_block_at(rectmul_block const *blocks, long stride, long row, long col) -> rectmul_block const * {
  return blocks + static_cast<std::size_t>(row * stride + col);
}

inline void rectmul_init_matrix(rectmul_block *R, long x, long y, long stride, double value) {
  for (long i = 0; i < x; ++i) {
    for (long j = 0; j < y; ++j) {
      rectmul_block_at(R, stride, i, j)->fill(value);
    }
  }
}

inline auto rectmul_check_matrix(rectmul_block const *R, long x, long y, long stride, double value) -> bool {
  for (long i = 0; i < x; ++i) {
    for (long j = 0; j < y; ++j) {
      auto const &block = *rectmul_block_at(R, stride, i, j);
      for (double actual : block) {
        if (std::abs(actual - value) > 0.0) {
          return false;
        }
      }
    }
  }
  return true;
}

inline auto rectmul_make(long n) -> rectmul_problem {
  long x = n / rectmul_block_edge;
  long y = n / rectmul_block_edge;
  long z = n / rectmul_block_edge;
  rectmul_problem problem{
      .A = std::vector<rectmul_block>(static_cast<std::size_t>(x * y)),
      .B = std::vector<rectmul_block>(static_cast<std::size_t>(y * z)),
      .R = std::vector<rectmul_block>(static_cast<std::size_t>(x * z)),
      .x = x,
      .y = y,
      .z = z,
  };
  rectmul_init_matrix(problem.A.data(), x, y, y, 1.0);
  rectmul_init_matrix(problem.B.data(), y, z, z, 1.0);
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
  state.counters["block"] = static_cast<double>(rectmul_block_edge);

  lf_bench::bench(state, threads, true, [&]() -> bool {
    state.PauseTiming();
    rectmul_init_matrix(problem.R.data(), problem.x, problem.z, problem.z, 0.0);
    state.ResumeTiming();

    std::invoke(fn, problem.A.data(), problem.y, problem.B.data(), problem.z, problem.x, problem.y, problem.z,
                problem.R.data(), problem.z, 0);

    benchmark::DoNotOptimize(problem.R.data());
    return rectmul_check_matrix(problem.R.data(), problem.x, problem.z, problem.z,
                                static_cast<double>(problem.y * rectmul_block_edge));
  });
}

template <typename Fn>
void run_rectmul(benchmark::State &state, Fn fn) {
  run_rectmul(state, lf_bench::no_threads, fn);
}
