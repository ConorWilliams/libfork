#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
  #include <vector>
#else
import std;
#endif

inline constexpr int heat_test = 64;
inline constexpr int heat_base = 1024;

inline constexpr int heat_iters = 16;

// Initialise grid with a fixed analytic profile (boundaries clamped).
inline auto heat_make_grid(int n) -> std::vector<double> {
  std::vector<double> g(static_cast<std::size_t>(n) * n);
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      double dx = static_cast<double>(x) / (n - 1) - 0.5;
      double dy = static_cast<double>(y) / (n - 1) - 0.5;
      g[static_cast<std::size_t>(y) * n + x] = std::exp(-8.0 * (dx * dx + dy * dy));
    }
  }
  return g;
}
