#pragma once

#include <benchmark/benchmark.h>

#include "macros.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <concepts>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <new>
  #include <ranges>
  #include <span>
  #include <type_traits>
  #include <vector>
#else
import std;
#endif

inline constexpr std::int64_t fold_test = 10;

inline constexpr std::int64_t fold_1024 = 1'024;
inline constexpr std::int64_t fold_1024_base = fold_1024;
inline constexpr std::int64_t fold_1024_sq_base = fold_1024 * fold_1024;
inline constexpr std::int64_t fold_1024_cu_base = fold_1024 * fold_1024 * fold_1024;

enum class fold_data_mode : char { memory, lazy };
enum class fold_chunk_mode : char { explicit_one, deduced, fixed };
enum class fold_projection_mode : char { sync, async };

template <typename T>
constexpr auto fold_value(std::size_t index) -> T {
  return static_cast<T>(index % 4UZ);
}

template <typename T>
constexpr auto make_fold_range(std::size_t count) {
  return std::views::iota(std::size_t{}, count) | std::views::transform([](std::size_t index) -> T {
           return fold_value<T>(index);
         });
}

template <typename T>
using fold_accum_t = std::conditional_t<std::same_as<T, float>, double, std::int64_t>;

template <typename T>
constexpr auto expected_fold_result(std::size_t count) -> fold_accum_t<T> {
  auto groups = count / 4UZ;
  auto remainder = count % 4UZ;
  return static_cast<fold_accum_t<T>>((groups * 6UZ) + ((remainder * (remainder - 1UZ)) / 2UZ));
}

template <typename T>
auto fold_result_is_correct(fold_accum_t<T> result, fold_accum_t<T> expect) -> bool {
  if constexpr (std::floating_point<fold_accum_t<T>>) {
    return std::abs(result - expect) <= 1e-6;
  } else {
    return result == expect;
  }
}

template <fold_data_mode Data, typename T, typename Fn>
void run_fold_input(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  auto expect = expected_fold_result<T>(n);

  auto run = [&](auto const &range) -> void {
    lf_bench::bench(state, threads, expect, fold_result_is_correct<T>, [&]() -> fold_accum_t<T> {
      return std::invoke(fn, range);
    });
  };

  if constexpr (Data == fold_data_mode::memory) {
    run(make_fold_range<T>(n) | std::ranges::to<std::vector<T>>());
  } else {
    run(make_fold_range<T>(n));
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

template <fold_data_mode Data, typename T, typename Fn>
void run_fold_input(benchmark::State &state, Fn fn) {
  run_fold_input<Data, T>(state, lf_bench::no_threads, fn);
}

// Use alias for shorted names.
inline constexpr auto memory = fold_data_mode::memory;
inline constexpr auto lazy = fold_data_mode::lazy;
inline constexpr auto chunk_1 = fold_chunk_mode::explicit_one;
inline constexpr auto chunk_deduced = fold_chunk_mode::deduced;
inline constexpr auto chunk_fixed = fold_chunk_mode::fixed;
inline constexpr auto sync_proj = fold_projection_mode::sync;
inline constexpr auto async_proj = fold_projection_mode::async;

using int32 = std::int32_t;
using float32 = float;

#define LF_FOLD_BENCH_SIZES_SMALL(bench_fn, category, name, ...)                                             \
  BENCH_ONE(bench_fn, category, name, test, fold __VA_OPT__(, ) __VA_ARGS__)                                 \
  BENCH_ONE(bench_fn, category, name, base, fold_1024 __VA_OPT__(, ) __VA_ARGS__)                            \
  BENCH_ONE(bench_fn, category, name, base, fold_1024_sq __VA_OPT__(, ) __VA_ARGS__)

#define LF_FOLD_BENCH_SIZES(bench_fn, category, name, ...)                                                   \
  LF_FOLD_BENCH_SIZES_SMALL(bench_fn, category, name __VA_OPT__(, ) __VA_ARGS__)                             \
  BENCH_ONE(bench_fn, category, name, base, fold_1024_cu __VA_OPT__(, ) __VA_ARGS__)

#define LF_FOLD_BENCH_SIZES_MT(bench_fn, category, name, ...)                                                \
  BENCH_ONE_MT(bench_fn, category, name, test, fold __VA_OPT__(, ) __VA_ARGS__)                              \
  BENCH_ONE_MT(bench_fn, category, name, base, fold_1024_cu __VA_OPT__(, ) __VA_ARGS__)
