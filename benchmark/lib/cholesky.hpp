#pragma once

#include <algorithm>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <memory>
#else
import std;
#endif

inline constexpr unsigned cholesky_test = 64;
inline constexpr unsigned cholesky_base = 1024;

inline constexpr unsigned cholesky_cutoff = 32;
inline constexpr unsigned cholesky_bandwidth = 8;

struct cholesky_problem {
  std::unique_ptr<double[]> initial;
  std::unique_ptr<double[]> matrix;
  std::unique_ptr<double[]> lower;
  unsigned n;
};

struct cholesky_output {
  double const *matrix;
  double const *lower;
  unsigned n;
  unsigned stride;
};

inline auto cholesky_lower_value(unsigned i, unsigned j) -> double {
  if (i == j) {
    return 3.0 + static_cast<double>((i % 13U) + 1U) / 16.0;
  }
  if (i < j || i - j > cholesky_bandwidth) {
    return 0.0;
  }
  return static_cast<double>(((i + 5U) * (j + 3U)) % 23U + 1U) / 128.0;
}

inline auto cholesky_make(unsigned n) -> cholesky_problem {
  auto total = static_cast<std::size_t>(n) * n;
  cholesky_problem problem{
      .initial = std::make_unique<double[]>(total),
      .matrix = std::make_unique<double[]>(total),
      .lower = std::make_unique<double[]>(total),
      .n = n,
  };

  for (std::size_t i = 0; i < total; ++i) {
    problem.initial[i] = 0.0;
    problem.lower[i] = 0.0;
  }

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j <= i; ++j) {
      problem.lower[static_cast<std::size_t>(i) * n + j] = cholesky_lower_value(i, j);
    }
  }

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j <= i; ++j) {
      double value = 0.0;
      unsigned first = i > cholesky_bandwidth ? i - cholesky_bandwidth : 0;
      unsigned last = std::min(j, n - 1);
      for (unsigned k = first; k <= last; ++k) {
        if (j - k <= cholesky_bandwidth) {
          value += problem.lower[static_cast<std::size_t>(i) * n + k] *
                   problem.lower[static_cast<std::size_t>(j) * n + k];
        }
      }
      problem.initial[static_cast<std::size_t>(i) * n + j] = value;
      problem.initial[static_cast<std::size_t>(j) * n + i] = value;
    }
  }

  return problem;
}

inline void cholesky_basecase(double *A, unsigned n, unsigned s) {
  for (unsigned k = 0; k < n; ++k) {
    double diag = std::sqrt(A[k * s + k]);
    A[k * s + k] = diag;
    for (unsigned i = k + 1; i < n; ++i) {
      A[i * s + k] /= diag;
    }
    for (unsigned j = k + 1; j < n; ++j) {
      for (unsigned i = j; i < n; ++i) {
        A[i * s + j] -= A[i * s + k] * A[j * s + k];
      }
    }
  }
}

inline void cholesky_solve_rows(
    double *A10, double const *L00, unsigned row_begin, unsigned row_end, unsigned m, unsigned s) {
  for (unsigned row = row_begin; row < row_end; ++row) {
    double *B = A10 + static_cast<std::size_t>(row) * s;
    for (unsigned j = 0; j < m; ++j) {
      for (unsigned k = 0; k < j; ++k) {
        B[j] -= L00[j * s + k] * B[k];
      }
      B[j] /= L00[j * s + j];
    }
  }
}

inline void cholesky_schur_rows(
    double *A11, double const *A10, unsigned row_begin, unsigned row_end, unsigned m, unsigned s) {
  for (unsigned i = row_begin; i < row_end; ++i) {
    for (unsigned j = 0; j <= i; ++j) {
      double sum = 0.0;
      for (unsigned k = 0; k < m; ++k) {
        sum += A10[static_cast<std::size_t>(i) * s + k] * A10[static_cast<std::size_t>(j) * s + k];
      }
      A11[static_cast<std::size_t>(i) * s + j] -= sum;
    }
  }
}

inline auto cholesky_is_close(cholesky_output out, double tolerance) -> bool {
  double error = 0.0;
  for (unsigned i = 0; i < out.n; ++i) {
    for (unsigned j = 0; j <= i; ++j) {
      double actual = out.matrix[static_cast<std::size_t>(i) * out.stride + j];
      double expect = out.lower[static_cast<std::size_t>(i) * out.n + j];
      error = std::max(error, std::abs(actual - expect));
    }
  }
  return error <= tolerance;
}

template <typename Fn>
void run_cholesky(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<unsigned>(state.range(0));
  auto problem = cholesky_make(n);
  auto total = static_cast<std::size_t>(n) * n;

  state.counters["n"] = n;
  state.counters["cutoff"] = cholesky_cutoff;

  lf_bench::bench(state, threads, 1e-8, cholesky_is_close, [&]() -> cholesky_output {
    state.PauseTiming();
    std::copy_n(problem.initial.get(), total, problem.matrix.get());
    state.ResumeTiming();

    std::invoke(fn, problem.matrix.get(), n, n);

    return {.matrix = problem.matrix.get(), .lower = problem.lower.get(), .n = n, .stride = n};
  });
}

template <typename Fn>
void run_cholesky(benchmark::State &state, Fn fn) {
  run_cholesky(state, lf_bench::no_threads, fn);
}
