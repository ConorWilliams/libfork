#pragma once

#include <benchmark/benchmark.h>

#include "macros.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <concepts>
  #include <cstddef>
  #include <cstdint>
  #include <format>
  #include <functional>
  #include <new>
  #include <ranges>
  #include <span>
  #include <type_traits>
  #include <utility>
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
enum class fold_chunk_mode : char { explicit_one, deduced, k1000 };
enum class fold_projection_mode : char { sync, async };

template <typename T>
constexpr auto fold_value(std::size_t index) -> T {
  return static_cast<T>(index % 4UZ);
}

template <typename T>
constexpr auto make_fold_range(std::size_t count) {
  return std::views::iota(std::size_t{}, count) | std::views::transform([](std::size_t index) {
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

inline void set_fold_throughput(benchmark::State &state, std::size_t n, std::size_t bytes_per_item) {
  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
  state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(n * bytes_per_item));
}

template <fold_data_mode Data, typename T, typename Fn>
void run_fold_input(benchmark::State &state, Fn &&fn) {

  auto n = static_cast<std::size_t>(state.range(0));
  auto expect = expected_fold_result<T>(n);

  if constexpr (Data == fold_data_mode::memory) {

    std::vector<T> values(n);
    for (std::size_t i = 0; i < n; ++i) {
      values[i] = fold_value<T>(i);
    }

    for (auto _ : state) {
      benchmark::DoNotOptimize(values.data());
      auto result = std::invoke(fn, std::span{values});
      if (!fold_result_is_correct<T>(result, expect)) {
        state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
        break;
      }
      benchmark::DoNotOptimize(result);
    }

  } else {

    auto values = make_fold_range<T>(n);

    for (auto _ : state) {
      benchmark::DoNotOptimize(values);
      auto result = std::invoke(fn, values);
      if (!fold_result_is_correct<T>(result, expect)) {
        state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
        break;
      }
      benchmark::DoNotOptimize(result);
    }
  }

  set_fold_throughput(state, n, sizeof(T));
}

// Use alias for shorted names.
inline constexpr auto memory = fold_data_mode::memory;
inline constexpr auto lazy = fold_data_mode::lazy;
inline constexpr auto chunk_1 = fold_chunk_mode::explicit_one;
inline constexpr auto chunk_deduced = fold_chunk_mode::deduced;
inline constexpr auto chunk_1000 = fold_chunk_mode::k1000;
inline constexpr auto sync_proj = fold_projection_mode::sync;
inline constexpr auto async_proj = fold_projection_mode::async;

using int32 = std::int32_t;
using float32 = float;

#define LF_FOLD_BENCH_SIZES(bench_fn, category, name, ...)                                                   \
  BENCH_ONE(bench_fn, category, name, test, fold __VA_OPT__(, ) __VA_ARGS__)                                 \
  BENCH_ONE(bench_fn, category, name, base, fold_1024 __VA_OPT__(, ) __VA_ARGS__)                            \
  BENCH_ONE(bench_fn, category, name, base, fold_1024_sq __VA_OPT__(, ) __VA_ARGS__)                         \
  BENCH_ONE(bench_fn, category, name, base, fold_1024_cu __VA_OPT__(, ) __VA_ARGS__)
