#pragma once

#include <benchmark/benchmark.h>

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
  #include <format>
  #include <functional>
  #include <utility>
#else
import std;
#endif

namespace lf_bench {

inline auto inverse_complexity(benchmark::IterationCount n) -> double { return 1.0 / static_cast<double>(n); }

template <typename Expected, typename Fn, typename Check = std::equal_to<>>
void bench(benchmark::State &state, const Expected &expected, Fn &&fn, Check &&check = {}) {
  for (auto _ : state) {
    auto result = std::invoke(fn);

    if (!std::invoke(check, result, expected)) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expected));
      break;
    }

    benchmark::DoNotOptimize(result);
  }
}

template <typename Expected, typename Fn, typename Check = std::equal_to<>>
void bench_mt(
    benchmark::State &state, std::int64_t threads, const Expected &expected, Fn &&fn, Check &&check = {}) {
  state.counters["p"] = static_cast<double>(threads);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads));

  bench(state, expected, std::forward<Fn>(fn), std::forward<Check>(check));
}

} // namespace lf_bench
