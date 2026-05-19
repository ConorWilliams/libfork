#include <benchmark/benchmark.h>

#include "fft.hpp"
#include "macros.hpp"

import std;

namespace {

template <typename = void>
void fft_serial_bench(benchmark::State &state) {
  run_fft(state, [](fft_complex *input, fft_complex *output, fft_complex const *roots, unsigned n) {
    fft_mixed_serial(input, output, roots, n, n);
  });
}

} // namespace

BENCH_ALL(fft_serial_bench, serial, fft, fft)
