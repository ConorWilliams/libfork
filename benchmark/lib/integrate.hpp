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
inline constexpr double integrate_upper = 10'000.0;
inline constexpr double integrate_alpha = 0.01;
inline constexpr double integrate_center = 7'234.5;

struct integrate_result {
  double area;
  std::int64_t leaves;
  int depth;
};

template <>
struct std::formatter<integrate_result> : std::formatter<std::string> {
  auto format(const integrate_result &r, auto &ctx) const {
    return std::formatter<std::string>::format(
        std::format("{{area={}, leaves={}, depth={}}}", r.area, r.leaves, r.depth), ctx);
  }
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
  return 1.0 / (1.0 + integrate_alpha * distance * distance);
}

constexpr auto integrate_exact(double a, double b) -> double {
  auto indefinite = [](double x) {
    double scale = std::sqrt(integrate_alpha);
    return std::atan(scale * (x - integrate_center)) / scale;
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

  state.counters["epsilon"] = tolerance;
  state.counters["n"] = integrate_upper;

  for (auto _ : state) {
    auto result = std::invoke(fn, tolerance);

    state.PauseTiming();

    state.counters["depth"] = result.depth;
    state.counters["leaves"] = static_cast<double>(result.leaves);

    if (!integrate_is_close(result, expect)) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      state.ResumeTiming();
      break;
    }

    state.ResumeTiming();

    benchmark::DoNotOptimize(result);
  }
}
