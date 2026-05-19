#pragma once

#include "matrix.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <bit>
  #include <cstddef>
  #include <cstdint>
  #include <memory>
#else
import std;
#endif

inline constexpr unsigned transpose_test = 64;
inline constexpr unsigned transpose_base = 8192;

inline constexpr unsigned transpose_cutoff = 32;

static_assert(std::has_single_bit(transpose_test));
static_assert(std::has_single_bit(transpose_base));

struct transpose_args {
  matrix_buffer_t input;
  matrix_buffer_t A;
  unsigned n;
};

struct transpose_output {
  float const *A;
  unsigned n;
};

inline auto transpose_init(unsigned n) -> transpose_args {
  transpose_args args{
      .input = std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      .A = std::make_unique<float[]>(static_cast<std::size_t>(n) * n),
      .n = n,
  };

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      args.input[static_cast<std::size_t>(i) * n + j] = matrix_lhs_value(i, j);
    }
  }

  return args;
}

inline void transpose_reset(transpose_args &args) {
  std::copy_n(args.input.get(), static_cast<std::size_t>(args.n) * args.n, args.A.get());
}

inline auto check_transpose(transpose_output output, float const *) -> bool {
  constexpr float epsilon = 1e-6F;
  unsigned const n = output.n;

  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      float const actual = output.A[static_cast<std::size_t>(i) * n + j];
      float const expected = matrix_lhs_value(j, i);
      if (std::abs(actual - expected) > epsilon) {
        return false;
      }
    }
  }

  return true;
}

inline void transpose_basecase(float *A, unsigned n, unsigned s) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < i; ++j) {
      std::swap(A[static_cast<std::size_t>(i) * s + j], A[static_cast<std::size_t>(j) * s + i]);
    }
  }
}

inline void transpose_swap_basecase(float *A, float *B, unsigned n, unsigned s) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      std::swap(A[static_cast<std::size_t>(i) * s + j], B[static_cast<std::size_t>(j) * s + i]);
    }
  }
}

template <typename Fn>
void run_transpose(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<unsigned>(state.range(0));
  state.counters["n"] = n;

  auto args = transpose_init(n);

  lf_bench::bench(state, threads, args.input.get(), check_transpose, [&]() -> transpose_output {
    state.PauseTiming();
    transpose_reset(args);
    state.ResumeTiming();

    std::invoke(fn, args.A.get(), n);

    return {.A = args.A.get(), .n = n};
  });
}

template <typename Fn>
void run_transpose(benchmark::State &state, Fn fn) {
  run_transpose(state, lf_bench::no_threads, fn);
}
