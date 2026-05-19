#pragma once

#include <algorithm>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <memory>
  #include <vector>
#else
import std;
#endif

inline constexpr unsigned lu_test = 256;
inline constexpr unsigned lu_base = 4096;

inline constexpr unsigned lu_block_size = 16;
inline constexpr unsigned lu_bandwidth = 8;

using lu_block = std::array<double, lu_block_size * lu_block_size>;
using lu_matrix = std::vector<lu_block>;

struct lu_problem {
  lu_matrix initial;
  lu_matrix matrix;
  std::unique_ptr<double[]> lower;
  std::unique_ptr<double[]> upper;
  unsigned n;
  unsigned blocks;
};

struct lu_output {
  lu_matrix const *matrix;
  double const *lower;
  double const *upper;
  unsigned n;
  unsigned blocks;
};

inline auto lu_block_at(lu_block *matrix, unsigned stride, unsigned i, unsigned j) -> lu_block * {
  return matrix + static_cast<std::size_t>(i) * stride + j;
}

inline auto lu_block_at(lu_block const *matrix, unsigned stride, unsigned i, unsigned j) -> lu_block const * {
  return matrix + static_cast<std::size_t>(i) * stride + j;
}

inline auto lu_elem(lu_block const *matrix, unsigned blocks, unsigned i, unsigned j) -> double {
  auto const &block = *lu_block_at(matrix, blocks, i / lu_block_size, j / lu_block_size);
  return block[(i % lu_block_size) * lu_block_size + (j % lu_block_size)];
}

inline auto lu_elem(lu_block *matrix, unsigned blocks, unsigned i, unsigned j) -> double & {
  auto &block = *lu_block_at(matrix, blocks, i / lu_block_size, j / lu_block_size);
  return block[(i % lu_block_size) * lu_block_size + (j % lu_block_size)];
}

inline auto lu_lower_value(unsigned i, unsigned j) -> double {
  if (i == j) {
    return 1.0;
  }
  if (i < j || i - j > lu_bandwidth) {
    return 0.0;
  }
  return static_cast<double>(((i + 3U) * (j + 5U)) % 17U + 1U) / 128.0;
}

inline auto lu_upper_value(unsigned i, unsigned j) -> double {
  if (i > j || j - i > lu_bandwidth) {
    return 0.0;
  }
  if (i == j) {
    return 4.0 + static_cast<double>((i % 11U) + 1U) / 16.0;
  }
  return static_cast<double>(((i + 7U) * (j + 11U)) % 19U + 1U) / 96.0;
}

inline auto lu_make(unsigned n) -> lu_problem {
  unsigned blocks = n / lu_block_size;
  lu_problem problem{
      .initial = lu_matrix(static_cast<std::size_t>(blocks) * blocks),
      .matrix = lu_matrix(static_cast<std::size_t>(blocks) * blocks),
      .lower = std::make_unique<double[]>(static_cast<std::size_t>(n) * n),
      .upper = std::make_unique<double[]>(static_cast<std::size_t>(n) * n),
      .n = n,
      .blocks = blocks,
  };

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      problem.lower[static_cast<std::size_t>(i) * n + j] = lu_lower_value(i, j);
      problem.upper[static_cast<std::size_t>(i) * n + j] = lu_upper_value(i, j);
    }
  }

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      double value = 0.0;
      unsigned first = i > lu_bandwidth ? i - lu_bandwidth : 0;
      unsigned last = std::min({i, j, n - 1});
      for (unsigned k = first; k <= last; ++k) {
        if (j - k <= lu_bandwidth) {
          value += problem.lower[static_cast<std::size_t>(i) * n + k] *
                   problem.upper[static_cast<std::size_t>(k) * n + j];
        }
      }
      lu_elem(problem.initial.data(), blocks, i, j) = value;
    }
  }

  return problem;
}

inline void lu_elem_daxmy(double a, double const *x, double *y, unsigned n) {
  for (unsigned i = 0; i < n; ++i) {
    y[i] -= a * x[i];
  }
}

inline void lu_block_lu(lu_block &B) {
  for (unsigned k = 0; k < lu_block_size; ++k) {
    for (unsigned i = k + 1; i < lu_block_size; ++i) {
      B[i * lu_block_size + k] /= B[k * lu_block_size + k];
      lu_elem_daxmy(B[i * lu_block_size + k],
                    &B[k * lu_block_size + k + 1],
                    &B[i * lu_block_size + k + 1],
                    lu_block_size - k - 1);
    }
  }
}

inline void lu_block_lower_solve(lu_block &B, lu_block const &L) {
  for (unsigned i = 1; i < lu_block_size; ++i) {
    for (unsigned k = 0; k < i; ++k) {
      lu_elem_daxmy(L[i * lu_block_size + k], &B[k * lu_block_size], &B[i * lu_block_size], lu_block_size);
    }
  }
}

inline void lu_block_upper_solve(lu_block &B, lu_block const &U) {
  for (unsigned i = 0; i < lu_block_size; ++i) {
    for (unsigned k = 0; k < lu_block_size; ++k) {
      B[i * lu_block_size + k] /= U[k * lu_block_size + k];
      lu_elem_daxmy(B[i * lu_block_size + k],
                    U.data() + k * lu_block_size + k + 1,
                    B.data() + i * lu_block_size + k + 1,
                    lu_block_size - k - 1);
    }
  }
}

inline void lu_block_schur(lu_block &B, lu_block const &A, lu_block const &C) {
  for (unsigned i = 0; i < lu_block_size; ++i) {
    for (unsigned k = 0; k < lu_block_size; ++k) {
      lu_elem_daxmy(A[i * lu_block_size + k], &C[k * lu_block_size], &B[i * lu_block_size], lu_block_size);
    }
  }
}

inline auto lu_is_close(lu_output out, double tolerance) -> bool {
  double error = 0.0;
  auto const *matrix = out.matrix->data();
  for (unsigned i = 0; i < out.n; ++i) {
    for (unsigned j = 0; j < out.n; ++j) {
      double expect = i > j ? out.lower[static_cast<std::size_t>(i) * out.n + j]
                            : out.upper[static_cast<std::size_t>(i) * out.n + j];
      error = std::max(error, std::abs(lu_elem(matrix, out.blocks, i, j) - expect));
    }
  }
  return error <= tolerance;
}

template <typename Fn>
void run_lu(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<unsigned>(state.range(0));
  auto problem = lu_make(n);

  state.counters["n"] = n;
  state.counters["block"] = lu_block_size;

  lf_bench::bench(state, threads, 1e-8, lu_is_close, [&]() -> lu_output {
    state.PauseTiming();
    problem.matrix = problem.initial;
    state.ResumeTiming();

    std::invoke(fn, problem.matrix.data(), problem.blocks);

    return {
        .matrix = &problem.matrix,
        .lower = problem.lower.get(),
        .upper = problem.upper.get(),
        .n = problem.n,
        .blocks = problem.blocks,
    };
  });
}

template <typename Fn>
void run_lu(benchmark::State &state, Fn fn) {
  run_lu(state, lf_bench::no_threads, fn);
}
