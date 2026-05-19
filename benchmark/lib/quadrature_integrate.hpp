#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cmath>
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr std::int64_t quadrature_integrate_test = 64;
inline constexpr std::int64_t quadrature_integrate_base = 10'000;
inline constexpr std::int64_t quadrature_test = quadrature_integrate_test;
inline constexpr std::int64_t quadrature_base = quadrature_integrate_base;

inline constexpr double quadrature_integrate_epsilon = 1.0e-9;

struct quadrature_integrate_result {
  double area;
  std::int64_t leaves;
  int depth;
};

constexpr auto quadrature_integrate_fn(double x) -> double { return (x * x + 1.0) * x; }

constexpr auto quadrature_integrate_exact(double x) -> double { return x * x * x * x / 4.0 + x * x / 2.0; }

constexpr auto quadrature_integrate_area(double x1, double y1, double x2, double y2) -> double {
  return (y1 + y2) * (x2 - x1) / 2.0;
}

inline auto quadrature_integrate_is_close(quadrature_integrate_result result, double expect) -> bool {
  return std::abs(result.area - expect) <= 1e-8 * std::max(std::abs(expect), 1.0);
}

template <typename Fn>
void run_quadrature_integrate(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<double>(state.range(0));
  double y0 = quadrature_integrate_fn(0.0);
  double yn = quadrature_integrate_fn(n);
  double initial = quadrature_integrate_area(0.0, y0, n, yn);
  double expect = quadrature_integrate_exact(n);
  quadrature_integrate_result stats{};

  state.counters["epsilon"] = quadrature_integrate_epsilon;
  state.counters["n"] = n;

  lf_bench::bench(
      state, threads, expect, quadrature_integrate_is_close, [&]() -> quadrature_integrate_result {
        return stats = std::invoke(fn, 0.0, y0, n, yn, initial, 0);
      });

  state.counters["depth"] = stats.depth;
  state.counters["leaves"] = static_cast<double>(stats.leaves);
}

template <typename Fn>
void run_quadrature_integrate(benchmark::State &state, Fn fn) {
  run_quadrature_integrate(state, lf_bench::no_threads, fn);
}
