#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cmath>
  #include <cstdint>
  #include <format>
  #include <functional>
  #include <string>
#else
import std;
#endif

inline constexpr std::int64_t integrate_test = 6;
inline constexpr std::int64_t integrate_base = 9;

inline constexpr double integrate_lower = 0.0;
inline constexpr double integrate_upper = 1.0;
inline constexpr double integrate_center = 0.731;
inline constexpr double integrate_z = 1.0e-8;

struct integrate_result {
  double area;
  std::int64_t leaves;
  int depth;
};

constexpr auto integrate_tolerance(std::int64_t exponent) -> double {
  double result = 1.0;
  for (std::int64_t i = 0; i < exponent; ++i) {
    result *= 0.1;
  }
  return result;
}

constexpr auto integrate_fn(double x) -> double {
  double distance = x - integrate_center;
  return 1.0 / (integrate_z + distance * distance);
}

constexpr auto integrate_exact(double a, double b) -> double {
  auto indefinite = [](double x) {
    double scale = std::sqrt(integrate_z);
    return std::atan((x - integrate_center) / scale) / scale;
  };
  return indefinite(b) - indefinite(a);
}

inline auto integrate_is_close(const integrate_result &result, double expect) -> bool {
  return std::abs(result.area - expect) <= 1e-3 * std::abs(expect);
}

template <typename Fn>
void run_integrate(benchmark::State &state, Fn fn) {
  double tolerance = integrate_tolerance(state.range(0));
  double expect = integrate_exact(integrate_lower, integrate_upper);

  integrate_result stats{};

  state.counters["epsilon"] = tolerance;
  state.counters["n"] = integrate_upper;

  lf_bench::bench(state, expect, integrate_is_close, [&stats, tolerance, fn]() -> integrate_result {
    return stats = std::invoke(fn, tolerance);
  });

  state.counters["depth"] = stats.depth;
  state.counters["leaves"] = static_cast<double>(stats.leaves);
}
