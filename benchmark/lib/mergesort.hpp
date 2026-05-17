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

inline constexpr std::size_t mergesort_cutoff = 32;
inline constexpr std::size_t mergesort_merge_cutoff = 512;

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
