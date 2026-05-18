#include <benchmark/benchmark.h>

#include "nqueens.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct nqueens_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int const *a, int n, int d, int i) -> lf::task<std::int64_t, Context> {
    std::vector<int> aa(static_cast<std::size_t>(d + 1));

    for (int j = 0; j < d; ++j) {
      aa[static_cast<std::size_t>(j)] = a[j];

      int diff = a[j] - i;
      int dist = d - j;

      if (diff == 0 || dist == diff || dist + diff == 0) {
        co_return 0;
      }
    }

    if (d >= 0) {
      aa[static_cast<std::size_t>(d)] = i;
    }
    if (++d == n) {
      co_return 1;
    }

    std::vector<std::int64_t> results(static_cast<std::size_t>(n));
    int const *next = aa.data();

    auto sc = co_await lf::scope();
    for (int col = 0; col < n; ++col) {
      co_await sc.fork(&results[static_cast<std::size_t>(col)], nqueens_task{}, next, n, d, col);
    }
    co_await sc.join();

    std::int64_t sum = 0;
    for (std::int64_t value : results) {
      sum += value;
    }
    co_return sum;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_nqueens(state, threads, [&](int n) {
    return lf::schedule(scheduler, nqueens_task{}, nullptr, n, -1, 0).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, nqueens, nqueens, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, nqueens, nqueens, poly_busy_pool)
