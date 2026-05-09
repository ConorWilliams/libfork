#pragma once

#include <benchmark/benchmark.h>

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <concepts>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <iterator>
  #include <new>
  #include <span>
  #include <type_traits>
  #include <utility>
  #include <vector>
#else
import std;
#endif

namespace lf_bench {

inline constexpr std::int64_t fold_test = 10;
inline constexpr std::int64_t fold_one_gib = 1024LL * 1024LL * 1024LL;

template <typename T>
inline constexpr std::int64_t fold_one_gib_elements = fold_one_gib / static_cast<std::int64_t>(sizeof(T));

inline constexpr std::int64_t fold_10_base = 10;
inline constexpr std::int64_t fold_100_base = 100;
inline constexpr std::int64_t fold_1k_base = 1'000;
inline constexpr std::int64_t fold_10k_base = 10'000;
inline constexpr std::int64_t fold_100k_base = 100'000;
inline constexpr std::int64_t fold_1m_base = 1'000'000;
inline constexpr std::int64_t fold_10m_base = 10'000'000;
inline constexpr std::int64_t fold_100m_base = 100'000'000;
inline constexpr std::int64_t fold_1gib_base = fold_one_gib_elements<std::int32_t>;

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
struct ones_iterator {
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;

  difference_type pos = 0;

  constexpr auto operator*() const -> value_type { return value_type{1}; }
  constexpr auto operator[](difference_type) const -> value_type { return value_type{1}; }

  constexpr auto operator++() -> ones_iterator & {
    ++pos;
    return *this;
  }

  constexpr auto operator++(int) -> ones_iterator {
    auto ret = *this;
    ++*this;
    return ret;
  }

  constexpr auto operator--() -> ones_iterator & {
    --pos;
    return *this;
  }

  constexpr auto operator--(int) -> ones_iterator {
    auto ret = *this;
    --*this;
    return ret;
  }

  constexpr auto operator+=(difference_type n) -> ones_iterator & {
    pos += n;
    return *this;
  }

  constexpr auto operator-=(difference_type n) -> ones_iterator & {
    pos -= n;
    return *this;
  }

  friend constexpr auto operator+(ones_iterator it, difference_type n) -> ones_iterator {
    it += n;
    return it;
  }

  friend constexpr auto operator+(difference_type n, ones_iterator it) -> ones_iterator { return it + n; }

  friend constexpr auto operator-(ones_iterator it, difference_type n) -> ones_iterator {
    it -= n;
    return it;
  }

  friend constexpr auto operator-(ones_iterator lhs, ones_iterator rhs) -> difference_type {
    return lhs.pos - rhs.pos;
  }

  friend constexpr auto operator==(ones_iterator, ones_iterator) -> bool = default;
  friend constexpr auto operator<=>(ones_iterator, ones_iterator) = default;
};

template <typename T>
struct ones_range {
  using value_type = T;

  std::size_t count = 0;

  constexpr auto begin() const -> ones_iterator<T> { return {.pos = 0}; }

  constexpr auto end() const -> ones_iterator<T> {
    return {.pos = static_cast<typename ones_iterator<T>::difference_type>(count)};
  }

  constexpr auto size() const -> std::size_t { return count; }
};

template <typename T>
using fold_accum_t = std::conditional_t<std::same_as<T, float>, double, std::int64_t>;

template <typename T>
void set_fold_counters(benchmark::State &state, std::size_t n) {
  state.counters["n"] = static_cast<double>(n);
  state.counters["bytes"] = static_cast<double>(n * sizeof(T));
}

inline void set_fold_throughput(benchmark::State &state, std::size_t n, std::size_t bytes_per_item) {
  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
  state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(n * bytes_per_item));
}

template <fold_data_mode Data, typename T, typename Fn>
void run_fold_input(benchmark::State &state, Fn &&fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  set_fold_counters<T>(state, n);

  if constexpr (Data == fold_data_mode::memory) {
    std::vector<T> values;
    try {
      values.assign(n, T{1});
    } catch (std::bad_alloc const &) {
      state.SkipWithError("allocation failed");
      return;
    }

    for (auto _ : state) {
      benchmark::DoNotOptimize(values.data());
      auto result = std::invoke(fn, std::span<T>{values});
      benchmark::DoNotOptimize(result);
    }
  } else {
    auto values = ones_range<T>{.count = n};

    for (auto _ : state) {
      benchmark::DoNotOptimize(values);
      auto result = std::invoke(fn, values);
      benchmark::DoNotOptimize(result);
    }
  }

  set_fold_throughput(state, n, sizeof(T));
}

} // namespace lf_bench

inline constexpr std::int64_t fold_test = lf_bench::fold_test;
inline constexpr std::int64_t fold_10_base = lf_bench::fold_10_base;
inline constexpr std::int64_t fold_100_base = lf_bench::fold_100_base;
inline constexpr std::int64_t fold_1k_base = lf_bench::fold_1k_base;
inline constexpr std::int64_t fold_10k_base = lf_bench::fold_10k_base;
inline constexpr std::int64_t fold_100k_base = lf_bench::fold_100k_base;
inline constexpr std::int64_t fold_1m_base = lf_bench::fold_1m_base;
inline constexpr std::int64_t fold_10m_base = lf_bench::fold_10m_base;
inline constexpr std::int64_t fold_100m_base = lf_bench::fold_100m_base;
inline constexpr std::int64_t fold_1gib_base = lf_bench::fold_1gib_base;

inline constexpr auto memory = lf_bench::fold_data_mode::memory;
inline constexpr auto lazy = lf_bench::fold_data_mode::lazy;
inline constexpr auto chunk_1 = lf_bench::fold_chunk_mode::explicit_one;
inline constexpr auto chunk_deduced = lf_bench::fold_chunk_mode::deduced;
inline constexpr auto chunk_1000 = lf_bench::fold_chunk_mode::k1000;
inline constexpr auto sync_proj = lf_bench::fold_projection_mode::sync;
inline constexpr auto async_proj = lf_bench::fold_projection_mode::async;

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
  BENCH_ONE(bench_fn, category, name, base, fold_100m __VA_OPT__(, ) __VA_ARGS__)                            \
  BENCH_ONE(bench_fn, category, name, base, fold_1gib __VA_OPT__(, ) __VA_ARGS__)
