#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "winograd.hpp"

import std;

namespace {

template <typename = void>
void winograd_serial(benchmark::State &state) {
  run_matrix_multiply(state, 1e-3f, [](float const *A, float const *B, float *C, unsigned n) {
    winograd(A, n, B, n, C, n, n);
  });
}

} // namespace

BENCH_ALL(winograd_serial, serial, winograd, winograd)
