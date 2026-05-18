#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <cstdlib>
  #include <cstddef>
  #include <functional>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t quicksort_test = 10'000;
inline constexpr std::size_t quicksort_base = 100'000'000;

inline auto quicksort_make_input(std::size_t n) -> std::vector<int> {
  std::vector<int> out(n);
  std::srand(1);
  for (auto &v : out) {
    v = std::rand();
  }
  return out;
}

template <typename Fn>
void run_quicksort(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);

  std::vector<int> source = quicksort_make_input(n);
  std::vector<int> work(n);

  lf_bench::bench(state, threads, true, [&]() -> bool {
    work = source;
    std::invoke(fn, work.data(), work.size());
    benchmark::DoNotOptimize(work.data());
    return std::is_sorted(work.begin(), work.end());
  });
}

template <typename Fn>
void run_quicksort(benchmark::State &state, Fn fn) {
  run_quicksort(state, lf_bench::no_threads, fn);
}
