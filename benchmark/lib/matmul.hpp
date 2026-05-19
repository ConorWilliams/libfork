#pragma once

#include "matrix.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <bit>
  #include <cstdint>
#else
import std;
#endif

inline constexpr unsigned matmul_test = 64;
inline constexpr unsigned matmul_base = 2048;

inline constexpr unsigned matmul_cutoff = 8;

static_assert(std::has_single_bit(matmul_test));
static_assert(std::has_single_bit(matmul_base));

template <typename Fn>
void run_matmul(benchmark::State &state, std::int64_t threads, float max_relative_error, Fn fn) {
  run_matrix_multiply(state, threads, max_relative_error, fn);
}

template <typename Fn>
void run_matmul(benchmark::State &state, float max_relative_error, Fn fn) {
  run_matmul(state, lf_bench::no_threads, max_relative_error, fn);
}
