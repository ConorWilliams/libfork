#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cmath>
  #include <cstddef>
  #include <functional>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t heat_test = 64;
inline constexpr std::size_t heat_base = 1024;

inline constexpr std::size_t heat_iters = 16;

// Initialise grid with a fixed analytic profile (boundaries clamped).
inline auto heat_make_grid(std::size_t n) -> std::vector<double> {
  std::vector<double> g(n * n);
  for (std::size_t y = 0; y < n; ++y) {
    for (std::size_t x = 0; x < n; ++x) {
      double dx = static_cast<double>(x) / static_cast<double>(n - 1) - 0.5;
      double dy = static_cast<double>(y) / static_cast<double>(n - 1) - 0.5;
      g[y * n + x] = std::exp(-8.0 * (dx * dx + dy * dy));
    }
  }
  return g;
}

inline auto heat_matches(std::vector<double> const &actual, std::vector<double> const &expected) -> bool {
  for (std::size_t i = 0; i < actual.size(); ++i) {
    if (std::abs(actual[i] - expected[i]) > 1e-12) {
      return false;
    }
  }
  return true;
}

template <typename Fn>
void run_heat(benchmark::State &state, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["iters"] = static_cast<double>(heat_iters);

  std::vector<double> initial = heat_make_grid(n);
  std::vector<double> a(initial.size());
  std::vector<double> b(initial.size());

  a = initial;
  std::invoke(fn, a.data(), b.data(), n, heat_iters);
  std::vector<double> reference = (heat_iters % 2 == 0) ? a : b;

  lf_bench::bench(state, true, [&]() -> bool {
    a = initial;
    std::invoke(fn, a.data(), b.data(), n, heat_iters);
    benchmark::DoNotOptimize(a.data());
    return heat_matches((heat_iters % 2 == 0) ? a : b, reference);
  });
}
