#include <benchmark/benchmark.h>

#include "quicksort.hpp"

#include "helpers.hpp"

import std;

import libfork;

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

struct quicksort_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::uint32_t *first, std::uint32_t *last)
      -> lf::task<void, Context> {

    if (last - first <= static_cast<std::ptrdiff_t>(quicksort_basecase)) {
      insertion_sort(first, last);
      co_return;
    }

    auto *pivot = partition(first, last);

    auto sc = co_await lf::scope();
    co_await sc.fork(quicksort_task{}, pivot + 1, last);
    co_await sc.call(quicksort_task{}, first, pivot);
    co_await sc.join();
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_quicksort(state, [&](std::uint32_t *first, std::uint32_t *last) {
    lf::schedule(scheduler, quicksort_task{}, first, last).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, quicksort, quicksort, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, quicksort, quicksort, poly_busy_pool)
