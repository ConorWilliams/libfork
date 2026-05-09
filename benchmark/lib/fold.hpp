#pragma once

#include <benchmark/benchmark.h>

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <concepts>
  #include <cstddef>
  #include <cstdint>
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

inline constexpr std::int64_t fold_10_base = 10;
inline constexpr std::int64_t fold_100_base = 100;
inline constexpr std::int64_t fold_1k_base = 1'000;
inline constexpr std::int64_t fold_10k_base = 10'000;
inline constexpr std::int64_t fold_100k_base = 100'000;
inline constexpr std::int64_t fold_1m_base = 1'000'000;
inline constexpr std::int64_t fold_10m_base = 10'000'000;
inline constexpr std::int64_t fold_100m_base = 100'000'000;

enum class fold_data_mode {
  memory,
  lazy,
};

enum class fold_chunk_mode {
  explicit_one,
  deduced,
  k1000,
};

enum class fold_projection_mode { sync, async };

template <typename T>
constexpr auto make_ones_range(std::size_t count) {
  return std::views::iota(std::size_t{}, count) | std::views::transform([](std::size_t) {
           return T{1};
         });
}

template <typename T>
using fold_accum_t = std::conditional_t<std::same_as<T, float>, double, std::int64_t>;

inline void set_fold_throughput(benchmark::State &state, std::size_t n, std::size_t bytes_per_item) {
  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
  state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(n * bytes_per_item));
}

template <fold_data_mode Data, typename T, typename Fn>
void run_fold_input(benchmark::State &state, Fn &&fn) {

  auto n = static_cast<std::size_t>(state.range(0));

  if constexpr (Data == fold_data_mode::memory) {

    std::vector<T> values(n, T{1});

    for (auto _ : state) {
      benchmark::DoNotOptimize(values.data());
      auto result = std::invoke(fn, std::span{values});
      benchmark::DoNotOptimize(result);
    }

  } else {

    auto values = make_ones_range<T>(n);

    for (auto _ : state) {
      benchmark::DoNotOptimize(values);
      auto result = std::invoke(fn, values);
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
  BENCH_ONE(bench_fn, category, name, base, fold_10 __VA_OPT__(, ) __VA_ARGS__)                              \
  BENCH_ONE(bench_fn, category, name, base, fold_100 __VA_OPT__(, ) __VA_ARGS__)                             \
  BENCH_ONE(bench_fn, category, name, base, fold_1k __VA_OPT__(, ) __VA_ARGS__)                              \
  BENCH_ONE(bench_fn, category, name, base, fold_10k __VA_OPT__(, ) __VA_ARGS__)                             \
  BENCH_ONE(bench_fn, category, name, base, fold_100k __VA_OPT__(, ) __VA_ARGS__)                            \
  BENCH_ONE(bench_fn, category, name, base, fold_1m __VA_OPT__(, ) __VA_ARGS__)                              \
  BENCH_ONE(bench_fn, category, name, base, fold_10m __VA_OPT__(, ) __VA_ARGS__)                             \
  BENCH_ONE(bench_fn, category, name, base, fold_100m __VA_OPT__(, ) __VA_ARGS__)
