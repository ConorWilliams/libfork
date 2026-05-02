#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "mandelbrot.hpp"

import std;

namespace {

auto mandelbrot_compute(int n) -> std::uint64_t {
  std::uint64_t checksum = 0;
  for (int py = 0; py < n; ++py) {
    for (int px = 0; px < n; ++px) {
      checksum += static_cast<std::uint64_t>(mandelbrot_pixel(px, py, n));
    }
  }
  return checksum;
}

template <typename = void>
void mandelbrot_serial(benchmark::State &state) {

  int n = static_cast<int>(state.range(0));
  state.counters["n"] = n;

  std::uint64_t expect = mandelbrot_compute(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::uint64_t result = mandelbrot_compute(n);
    if (result != expect) {
      state.SkipWithError(std::format("mandelbrot checksum mismatch: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCH_ALL(mandelbrot_serial, serial, mandelbrot, mandelbrot)
