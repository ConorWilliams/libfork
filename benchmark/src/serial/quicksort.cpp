#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "quicksort.hpp"

import std;

namespace {

void quicksort(int *a, std::size_t n) {
  if (n < 2) {
    return;
  }

  int pivot = a[n / 2];
  int *left = a;
  int *right = a + n - 1;

  while (left <= right) {
    if (*left < pivot) {
      ++left;
    } else if (*right > pivot) {
      --right;
    } else {
      std::swap(*left, *right);
      ++left;
      --right;
    }
  }

  quicksort(a, static_cast<std::size_t>(right - a + 1));
  quicksort(left, static_cast<std::size_t>(a + n - left));
}

template <typename = void>
void quicksort_serial(benchmark::State &state) {
  run_quicksort(state, quicksort);
}

} // namespace

BENCH_ALL(quicksort_serial, serial, quicksort, quicksort)
