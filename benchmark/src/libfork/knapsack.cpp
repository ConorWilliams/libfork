#include <benchmark/benchmark.h>

#include "knapsack.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

void update_best(std::atomic<int> &best, int value) {
  int old = best.load(std::memory_order_acquire);
  while (old < value &&
         !best.compare_exchange_weak(old, value, std::memory_order_release, std::memory_order_acquire)) {
  }
}

struct knapsack_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         std::vector<knapsack_item> const *items,
                         std::size_t idx,
                         int cap,
                         int val,
                         std::atomic<int> *incumbent) -> lf::task<int, Context> {

    if (cap < 0) {
      co_return knapsack_empty;
    }

    if (idx == items->size() || cap == 0) {
      co_return val;
    }

    if (knapsack_upper_bound((*items)[idx], cap, val) < incumbent->load(std::memory_order_acquire)) {
      co_return knapsack_empty;
    }

    int with = knapsack_empty;
    int without = knapsack_empty;

    auto sc = co_await lf::scope();
    // Benchmark is order sensitive
    co_await sc.fork(&with,
                     knapsack_fn{},
                     items,
                     idx + 1,
                     cap - (*items)[idx].weight,
                     val + (*items)[idx].value,
                     incumbent);
    co_await sc.call(&without, knapsack_fn{}, items, idx + 1, cap, val, incumbent);
    co_await sc.join();

    int best = std::max(with, without);
    update_best(*incumbent, best);
    co_return best;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_knapsack(state, threads, [&](knapsack_problem const &problem) -> int {
    std::atomic<int> best{knapsack_empty};
    return lf::schedule(scheduler, knapsack_fn{}, &problem.items, 0, problem.capacity, 0, &best).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, knapsack, knapsack, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, knapsack, knapsack, poly_busy_pool)

// TODO: large sizes for all benchmarks
BENCH_ONE_MT(run, libfork, knapsack, large, knapsack, mono_busy_pool)
BENCH_ONE_MT(run, libfork, knapsack, large, knapsack, poly_busy_pool)
