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

  std::size_t n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["iters"] = static_cast<double>(heat_iters);

  std::vector<double> initial = heat_make_grid(n);
  std::vector<double> a(initial.size());
  std::vector<double> b(initial.size());

  // Reference (run once).
  a = initial;
  heat_run(a.data(), b.data(), n, heat_iters);
  std::vector<double> reference = (heat_iters % 2 == 0) ? a : b;

  for (auto _ : state) {
    a = initial;
    benchmark::DoNotOptimize(a.data());
    heat_run(a.data(), b.data(), n, heat_iters);
    benchmark::DoNotOptimize(a.data());
  }

  std::vector<double> const &final = (heat_iters % 2 == 0) ? a : b;
  for (std::size_t i = 0; i < final.size(); ++i) {
    if (std::abs(final[i] - reference[i]) > 1e-12) {
      state.SkipWithError("heat result diverged from reference");
      break;
    }
  }
}

} // namespace

BENCH_ALL(heat_serial, serial, heat, heat)
