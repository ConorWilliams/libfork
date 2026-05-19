#include <benchmark/benchmark.h>

#include "quicksort.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct quicksort_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int *a, std::size_t n) -> lf::task<void, Context> {
    if (n < 2) {
      co_return;
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

    auto sc = co_await lf::scope();
    co_await sc.fork(quicksort_task{}, a, static_cast<std::size_t>(right - a + 1));
    co_await sc.call(quicksort_task{}, left, static_cast<std::size_t>(a + n - left));
    co_await sc.join();
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_quicksort(state, threads, [&](int *data, std::size_t n) {
    lf::schedule(scheduler, quicksort_task{}, data, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, quicksort, quicksort, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, quicksort, quicksort, poly_busy_pool)
