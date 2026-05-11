#include <benchmark/benchmark.h>

#include "heat.hpp"
#include "macros.hpp"

import std;

namespace {

void heat_run(double *a, double *b, std::size_t n, std::size_t iters) {
  for (std::size_t t = 0; t < iters; ++t) {
    heat_jacobi_step(a, b, n);
    std::swap(a, b);
  }
}

template <typename = void>
void heat_serial(benchmark::State &state) {
  run_heat(state, heat_run);
}

} // namespace

BENCH_ALL(heat_serial, serial, heat, heat)
