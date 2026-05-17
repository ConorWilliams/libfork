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

  auto len12 = n / 2;
  auto len1 = len12 / 2;
  auto len2 = len12 - len1;
  auto len34 = n - len12;
  auto len3 = len34 / 2;
  auto len4 = len34 - len3;

  auto *a1 = first;
  auto *a2 = a1 + len1;
  auto *a3 = first + len12;
  auto *a4 = a3 + len3;

  auto *b1 = scratch;
  auto *b2 = b1 + len1;
  auto *b3 = scratch + len12;
  auto *b4 = b3 + len3;

  mergesort(a1, a2, b1);
  mergesort(a2, a2 + len2, b2);
  mergesort(a3, a4, b3);
  mergesort(a4, a4 + len4, b4);

  std::merge(a1, a2, a2, a2 + len2, b1);
  std::merge(a3, a4, a4, a4 + len4, b3);
  std::merge(b1, b1 + len12, b3, b3 + len34, first);
}

template <typename = void>
void mergesort_serial(benchmark::State &state) {
  run_mergesort(state, mergesort);
}

} // namespace

BENCH_ALL(mergesort_serial, serial, mergesort, mergesort)
