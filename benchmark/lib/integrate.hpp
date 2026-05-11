#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cmath>
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr std::int64_t integrate_test = 100;
inline constexpr std::int64_t integrate_base = 10'000;

inline constexpr double integrate_epsilon = 1.0e-9;

inline constexpr auto integrate_fn(double x) -> double { return (x * x + 1.0) * x; }

inline constexpr auto integrate_exact(double a, double b) -> double {
  auto indefinite = [](double x) {
    return 0.25 * x * x * (x * x + 2);
  };
  return indefinite(b) - indefinite(a);
}

inline auto integrate_is_close(double result, double expect) -> bool {
  return std::abs(result - expect) <= 1e-3 * std::abs(expect);
}

template <typename Fn>
void run_integrate(benchmark::State &state, Fn fn) {
  std::int64_t n = state.range(0);
  double upper = static_cast<double>(n);
  double expect = integrate_exact(0, upper);

  state.counters["n"] = static_cast<double>(n);

  lf_bench::bench(state, expect, integrate_is_close, [upper, fn]() -> double {
    return std::invoke(fn, upper);
  });
}
