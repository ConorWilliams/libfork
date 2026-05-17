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

inline constexpr std::size_t quicksort_test = 10'000;
inline constexpr std::size_t quicksort_base = 10'000'000;

inline constexpr std::size_t quicksort_basecase = 32;

inline auto
quicksort_make_input(std::size_t n, std::uint64_t seed = 0xDEADBEEF) -> std::vector<std::uint32_t> {
  std::vector<std::uint32_t> out(n);
  std::mt19937_64 rng{seed};
  std::uniform_int_distribution<std::uint32_t> dist;
  for (auto &v : out) {
    v = dist(rng);
  }
  return out;
}

template <typename Fn>
void run_quicksort(benchmark::State &state, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);

  std::vector<std::uint32_t> source = quicksort_make_input(n);
  std::vector<std::uint32_t> reference = source;
  std::sort(reference.begin(), reference.end());

  std::vector<std::uint32_t> work(n);

  lf_bench::bench(state, true, [&]() -> bool {
    work = source;
    std::invoke(fn, work.data(), work.data() + work.size());
    benchmark::DoNotOptimize(work.data());
    return work == reference;
  });
}
