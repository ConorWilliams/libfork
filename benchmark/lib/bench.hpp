#pragma once

#include <benchmark/benchmark.h>

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
  #include <format>
  #include <functional>
#else
import std;
#endif

namespace lf_bench {

inline constexpr std::int64_t no_threads = 0;

inline auto inverse_complexity(benchmark::IterationCount n) -> double { return 1.0 / static_cast<double>(n); }

inline void report_threads(benchmark::State &state, std::int64_t threads) {
  if (threads == no_threads) {
    return;
  }

  state.counters["p"] = static_cast<double>(threads);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads));
}

// `bench` reports mismatches with a `std::format` call that formats both
// `result` and `expected`, so `Expected` and `std::invoke_result_t<Fn>` must be
// formattable.
template <typename Expected, typename Check, typename Fn>
void bench(benchmark::State &state, std::int64_t threads, const Expected &expected, Check check, Fn fn) {
  report_threads(state, threads);

  for (auto _ : state) {

    auto result = std::invoke(fn);

    if (!std::invoke(check, result, expected)) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expected));
      break;
    }

    benchmark::DoNotOptimize(result);
  }
}

template <typename Expected, typename Fn>
void bench(benchmark::State &state, std::int64_t threads, const Expected &expected, Fn fn) {
  bench(state, threads, expected, std::equal_to<>{}, fn);
}

template <typename Expected, typename Check, typename Fn>
void bench(benchmark::State &state, const Expected &expected, Check check, Fn fn) {
  bench(state, no_threads, expected, check, fn);
}

template <typename Expected, typename Fn>
void bench(benchmark::State &state, const Expected &expected, Fn fn) {
  bench(state, no_threads, expected, fn);
}

} // namespace lf_bench
