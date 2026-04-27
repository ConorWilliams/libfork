#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "quicksort.hpp"

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

auto partition(std::uint32_t *first, std::uint32_t *last) -> std::uint32_t * {
  std::uint32_t *mid = first + (last - first) / 2;
  std::uint32_t pivot = *mid;
  std::swap(*mid, *(last - 1));

  auto *store = first;
  for (auto *it = first; it < last - 1; ++it) {
    if (*it < pivot) {
      std::swap(*it, *store);
      ++store;
    }
  }
  std::swap(*store, *(last - 1));
  return store;
}

void quicksort(std::uint32_t *first, std::uint32_t *last) {
  while (last - first > static_cast<std::ptrdiff_t>(quicksort_basecase)) {
    auto *pivot = partition(first, last);
    quicksort(pivot + 1, last);
    last = pivot;
  }
  insertion_sort(first, last);
}

template <typename = void>
void quicksort_serial(benchmark::State &state) {

  std::size_t n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);

  std::vector<std::uint32_t> source = quicksort_make_input(n);
  std::vector<std::uint32_t> reference = source;
  std::sort(reference.begin(), reference.end());

  std::vector<std::uint32_t> work(n);

  for (auto _ : state) {
    work = source;
    benchmark::DoNotOptimize(work.data());
    quicksort(work.data(), work.data() + work.size());
    benchmark::DoNotOptimize(work.data());
  }

  if (work != reference) {
    state.SkipWithError("quicksort produced wrong order");
  }
}

} // namespace

BENCH_ALL(quicksort_serial, serial, quicksort, quicksort)
