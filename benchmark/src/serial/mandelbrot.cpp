#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "mandelbrot.hpp"

import std;

namespace {

template <typename = void>
void mandelbrot_serial(benchmark::State &state) {
  run_mandelbrot(state, mandelbrot_checksum);
}

} // namespace

BENCH_ALL(mandelbrot_serial, serial, mandelbrot, mandelbrot)
