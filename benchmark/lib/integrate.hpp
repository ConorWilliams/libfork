#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cmath>
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr std::int64_t integrate_test = 6;
inline constexpr std::int64_t integrate_base = 9;

inline constexpr double integrate_lower = 0.0;
inline constexpr double integrate_upper = 10'000.0;

constexpr auto integrate_tolerance(std::int64_t exponent) -> double {
  double result = 1.0;
  for (std::int64_t i = 0; i < exponent; ++i) {
    result *= 0.1;
  }
  return result;
}

constexpr auto integrate_fn(double x) -> double {
  double x2 = x * x;
  return x2 * x2 + x;
}

constexpr auto integrate_exact(double a, double b) -> double {
  auto indefinite = [](double x) {
    double x2 = x * x;
    return 0.2 * x2 * x2 * x + 0.5 * x2;
  };
  return indefinite(b) - indefinite(a);
}

inline auto integrate_is_close(double result, double expect) -> bool {
  return std::abs(result - expect) <= 1e-3 * std::abs(expect);
}

template <typename Fn>
void run_integrate(benchmark::State &state, Fn fn) {
  double tolerance = integrate_tolerance(state.range(0));
  double expect = integrate_exact(integrate_lower, integrate_upper);

  state.counters["epsilon"] = tolerance;
  state.counters["n"] = integrate_upper;

  lf_bench::bench(state, expect, integrate_is_close, [tolerance, fn]() -> double {
    return std::invoke(fn, tolerance);
  });
}
