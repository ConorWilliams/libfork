#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cmath>
  #include <cstddef>
  #include <functional>
  #include <utility>
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

inline void heat_jacobi_step(double const *src, double *dst, std::size_t n) {
  for (std::size_t y = 1; y < n - 1; ++y) {
    for (std::size_t x = 1; x < n - 1; ++x) {
      std::size_t i = y * n + x;
      dst[i] = 0.25 * (src[i - 1] + src[i + 1] + src[i - n] + src[i + n]);
    }
  }
  for (std::size_t x = 0; x < n; ++x) {
    dst[x] = src[x];
    dst[(n - 1) * n + x] = src[(n - 1) * n + x];
  }
  for (std::size_t y = 0; y < n; ++y) {
    dst[y * n] = src[y * n];
    dst[y * n + (n - 1)] = src[y * n + (n - 1)];
  }
}

inline auto
heat_reference(std::vector<double> initial, std::size_t n, std::size_t iters) -> std::vector<double> {
  std::vector<double> scratch(initial.size());
  double *src = initial.data();
  double *dst = scratch.data();

  for (std::size_t t = 0; t < iters; ++t) {
    heat_jacobi_step(src, dst, n);
    std::swap(src, dst);
  }

  if (src == initial.data()) {
    return initial;
  }
  return scratch;
}

template <typename Fn>
void run_heat(benchmark::State &state, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["iters"] = static_cast<double>(heat_iters);

  std::vector<double> initial = heat_make_grid(n);
  std::vector<double> a(initial.size());
  std::vector<double> b(initial.size());
  std::vector<double> reference = heat_reference(initial, n, heat_iters);

  lf_bench::bench(state, true, [&]() -> bool {
    a = initial;
    std::invoke(fn, a.data(), b.data(), n, heat_iters);
    benchmark::DoNotOptimize(a.data());
    return heat_matches((heat_iters % 2 == 0) ? a : b, reference);
  });
}
