#include <benchmark/benchmark.h>

#include "mergesort.hpp"

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

struct mergesort_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::uint32_t *first, std::uint32_t *last, std::uint32_t *scratch)
      -> lf::task<void, Context> {

    auto n = last - first;

    if (n <= static_cast<std::ptrdiff_t>(mergesort_basecase)) {
      insertion_sort(first, last);
      co_return;
    }

    auto left = n / 2;
    auto *mid = first + left;
    auto *scratch_mid = scratch + left;

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(mergesort_task{}, first, mid, scratch);
      co_await sc.call(mergesort_task{}, mid, last, scratch_mid);
      co_await sc.join();
    }

    std::merge(first, mid, mid, last, scratch);
    std::move(scratch, scratch + n, first);
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_mergesort(state, [&](std::uint32_t *first, std::uint32_t *last, std::uint32_t *scratch) {
    lf::schedule(scheduler, mergesort_task{}, first, last, scratch).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, mergesort, mergesort, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, mergesort, mergesort, poly_busy_pool)
