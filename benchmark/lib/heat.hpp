#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
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
