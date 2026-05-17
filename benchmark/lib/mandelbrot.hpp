#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <complex>
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr int mandelbrot_test = 128;
inline constexpr int mandelbrot_base = 1024;

inline constexpr int mandelbrot_max_iter = 256;

inline constexpr double mandelbrot_x_min = -2.0;
inline constexpr double mandelbrot_x_max = 1.0;
inline constexpr double mandelbrot_y_min = -1.5;
inline constexpr double mandelbrot_y_max = 1.5;

inline constexpr auto mandelbrot_pixel(int px, int py, int n) -> int {
  double cr = mandelbrot_x_min + (mandelbrot_x_max - mandelbrot_x_min) * px / n;
  double ci = mandelbrot_y_min + (mandelbrot_y_max - mandelbrot_y_min) * py / n;

  double zr = 0;
  double zi = 0;
  int iter = 0;

  while (iter < mandelbrot_max_iter && zr * zr + zi * zi <= 4.0) {
    double zr_new = zr * zr - zi * zi + cr;
    zi = 2 * zr * zi + ci;
    zr = zr_new;
    ++iter;
  }

  return iter;
}

inline auto mandelbrot_checksum(int n) -> std::uint64_t {
  std::uint64_t checksum = 0;
  for (int py = 0; py < n; ++py) {
    for (int px = 0; px < n; ++px) {
      checksum += static_cast<std::uint64_t>(mandelbrot_pixel(px, py, n));
    }
  }
  return checksum;
}

template <typename Fn>
void run_mandelbrot(benchmark::State &state, Fn fn) {
  int n = static_cast<int>(state.range(0));
  std::uint64_t expect = mandelbrot_checksum(n);

  state.counters["n"] = n;

  lf_bench::bench(state, expect, [n, fn]() -> std::uint64_t {
    return std::invoke(fn, n);
  });
}
