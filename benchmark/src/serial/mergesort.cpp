#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "mergesort.hpp"

import std;

namespace {

void insertion_sort(std::uint32_t *first, std::uint32_t *last) {
  for (auto *it = first + 1; it < last; ++it) {
    std::uint32_t v = *it;
    auto *j = it;
    while (j > first && *(j - 1) > v) {
      *j = *(j - 1);
      --j;
    }
    *j = v;
  }
}

void mergesort(std::uint32_t *first, std::uint32_t *last, std::uint32_t *scratch) {
  auto n = last - first;

  if (n <= static_cast<std::ptrdiff_t>(mergesort_basecase)) {
    insertion_sort(first, last);
    return;
  }

  auto left = n / 2;
  auto *mid = first + left;
  auto *scratch_mid = scratch + left;

  mergesort(first, mid, scratch);
  mergesort(mid, last, scratch_mid);

  std::merge(first, mid, mid, last, scratch);
  std::move(scratch, scratch + n, first);
}

template <typename = void>
void mergesort_serial(benchmark::State &state) {
  run_mergesort(state, mergesort);
}

} // namespace

BENCH_ALL(mergesort_serial, serial, mergesort, mergesort)
