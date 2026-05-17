#include <benchmark/benchmark.h>

#include "mergesort.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct merge_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         std::uint32_t const *a_first,
                         std::uint32_t const *a_last,
                         std::uint32_t const *b_first,
                         std::uint32_t const *b_last,
                         std::uint32_t *out) -> lf::task<void, Context> {

    auto a_len = a_last - a_first;
    auto b_len = b_last - b_first;
    auto n = a_len + b_len;

    if (a_len == 0) {
      std::copy(b_first, b_last, out);
      co_return;
    }
    if (b_len == 0) {
      std::copy(a_first, a_last, out);
      co_return;
    }
    if (n <= static_cast<std::ptrdiff_t>(mergesort_parallel_basecase)) {
      std::merge(a_first, a_last, b_first, b_last, out);
      co_return;
    }

    std::uint32_t const *a_mid = nullptr;
    std::uint32_t const *b_mid = nullptr;

    if (a_len >= b_len) {
      a_mid = a_first + a_len / 2;
      b_mid = std::lower_bound(b_first, b_last, *a_mid);
    } else {
      b_mid = b_first + b_len / 2;
      a_mid = std::upper_bound(a_first, a_last, *b_mid);
    }

    auto *out_mid = out + (a_mid - a_first) + (b_mid - b_first);

    auto sc = co_await lf::scope();
    co_await sc.fork(merge_fn{}, a_first, a_mid, b_first, b_mid, out);
    co_await sc.call(merge_fn{}, a_mid, a_last, b_mid, b_last, out_mid);
    co_await sc.join();
  }
};

struct mergesort_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::uint32_t *first, std::uint32_t *last, std::uint32_t *scratch)
      -> lf::task<void, Context> {

    auto n = last - first;

    if (n <= static_cast<std::ptrdiff_t>(mergesort_parallel_basecase)) {
      mergesort_serial_sort(first, last, scratch);
      co_return;
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

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(mergesort_fn{}, a1, a2, b1);
      co_await sc.fork(mergesort_fn{}, a2, a2 + len2, b2);
      co_await sc.fork(mergesort_fn{}, a3, a4, b3);
      co_await sc.call(mergesort_fn{}, a4, a4 + len4, b4);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(merge_fn{}, a1, a2, a2, a2 + len2, b1);
      co_await sc.call(merge_fn{}, a3, a4, a4, a4 + len4, b3);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(merge_fn{}, b1, b1 + len12, b3, b3 + len34, first);
      co_await sc.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_mergesort(state, threads, [&](std::uint32_t *first, std::uint32_t *last, std::uint32_t *scratch) {
    lf::schedule(scheduler, mergesort_fn{}, first, last, scratch).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, mergesort, mergesort, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, mergesort, mergesort, poly_busy_pool)
