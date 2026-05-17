#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <random>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t mergesort_test = 10'000;
inline constexpr std::size_t mergesort_base = 10'000'000;

inline constexpr std::size_t mergesort_basecase = 32;
inline constexpr std::size_t mergesort_merge_basecase = 2'048;

inline auto
mergesort_make_input(std::size_t n, std::uint64_t seed = 0xBADC0DE) -> std::vector<std::uint32_t> {
  std::vector<std::uint32_t> out(n);
  std::mt19937_64 rng{seed};
  std::uniform_int_distribution<std::uint32_t> dist;
  for (auto &v : out) {
    v = dist(rng);
  }
  return out;
}

inline void mergesort_insertion_sort(std::uint32_t *first, std::uint32_t *last) {
  for (auto *it = first + 1; it < last; ++it) {
    std::uint32_t v = *it;
    auto *j = it;
    while (j > first && *(j - 1) > v) {
      *j = *(j - 1);
      --j;
    }
    *j = v;
  }
}

inline void mergesort_serial_sort(std::uint32_t *first, std::uint32_t *last, std::uint32_t *scratch) {
  auto n = last - first;

  if (n <= static_cast<std::ptrdiff_t>(mergesort_basecase)) {
    mergesort_insertion_sort(first, last);
    return;
  }

  auto len12 = n / 2;
  auto len1 = len12 / 2;
  auto len2 = len12 - len1;
  auto len34 = n - len12;
  auto len3 = len34 / 2;
  auto len4 = len34 - len3;

  auto *a1 = first;
  auto *a2 = a1 + len1;
  auto *a3 = first + len12;
  auto *a4 = a3 + len3;

  auto *b1 = scratch;
  auto *b2 = b1 + len1;
  auto *b3 = scratch + len12;
  auto *b4 = b3 + len3;

  mergesort_serial_sort(a1, a2, b1);
  mergesort_serial_sort(a2, a2 + len2, b2);
  mergesort_serial_sort(a3, a4, b3);
  mergesort_serial_sort(a4, a4 + len4, b4);

  std::merge(a1, a2, a2, a2 + len2, b1);
  std::merge(a3, a4, a4, a4 + len4, b3);
  std::merge(b1, b1 + len12, b3, b3 + len34, first);
}

template <typename Fn>
void run_mergesort(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);

  std::vector<std::uint32_t> source = mergesort_make_input(n);
  std::vector<std::uint32_t> reference = source;
  std::stable_sort(reference.begin(), reference.end());

  std::vector<std::uint32_t> work(n);
  std::vector<std::uint32_t> scratch(n);

  lf_bench::bench(state, threads, true, [&]() -> bool {
    work = source;
    std::invoke(fn, work.data(), work.data() + work.size(), scratch.data());
    benchmark::DoNotOptimize(work.data());
    return work == reference;
  });
}

template <typename Fn>
void run_mergesort(benchmark::State &state, Fn fn) {
  run_mergesort(state, lf_bench::no_threads, fn);
}
