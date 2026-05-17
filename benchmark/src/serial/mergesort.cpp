#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "mergesort.hpp"

import std;

namespace {

template <typename = void>
void mergesort_serial(benchmark::State &state) {
  run_mergesort(state, mergesort_serial_sort);
}

} // namespace

BENCH_ALL(mergesort_serial, serial, mergesort, mergesort)
