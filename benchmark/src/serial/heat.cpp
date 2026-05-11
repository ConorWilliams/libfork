#include <benchmark/benchmark.h>

#include "heat.hpp"
#include "macros.hpp"

import std;

namespace {

// Single Jacobi sweep with Dirichlet (boundary unchanged) condition.
void heat_step(double const *src, double *dst, std::size_t n) {
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

void heat_run(double *a, double *b, std::size_t n, std::size_t iters) {
  for (std::size_t t = 0; t < iters; ++t) {
    heat_step(a, b, n);
    std::swap(a, b);
  }
}

template <typename = void>
void heat_serial(benchmark::State &state) {
  run_heat(state, heat_run);
}

} // namespace

BENCH_ALL(heat_serial, serial, heat, heat)
