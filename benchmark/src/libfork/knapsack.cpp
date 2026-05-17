#include <benchmark/benchmark.h>

#include "knapsack.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

auto upper_bound(std::vector<knapsack_item> const &items,
                 std::size_t idx,
                 int remaining_cap,
                 int current_value) -> double {

  double bound = current_value;
  int cap = remaining_cap;

  for (std::size_t i = idx; i < items.size(); ++i) {
    if (items[i].weight <= cap) {
      cap -= items[i].weight;
      bound += items[i].value;
    } else {
      bound += static_cast<double>(items[i].value) * cap / items[i].weight;
      return bound;
    }
  }

  return bound;
}

void update_best(std::atomic<int> &best, int value) {
  int old = best.load(std::memory_order_relaxed);
  while (old < value &&
         !best.compare_exchange_weak(old, value, std::memory_order_relaxed, std::memory_order_relaxed)) {
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

    update_best(*incumbent, val);

    if (idx == items->size()) {
      co_return incumbent->load(std::memory_order_relaxed);
    }

    if (upper_bound(*items, idx, cap, val) <= incumbent->load(std::memory_order_relaxed)) {
      co_return incumbent->load(std::memory_order_relaxed);
    }

    int with = 0;
    int without = 0;

    auto sc = co_await lf::scope();

    if ((*items)[idx].weight <= cap) {
      co_await sc.fork(&with,
                       knapsack_fn{},
                       items,
                       idx + 1,
                       cap - (*items)[idx].weight,
                       val + (*items)[idx].value,
                       incumbent);

      if (upper_bound(*items, idx + 1, cap, val) > incumbent->load(std::memory_order_relaxed)) {
        co_await sc.call(&without, knapsack_fn{}, items, idx + 1, cap, val, incumbent);
      } else {
        without = incumbent->load(std::memory_order_relaxed);
      }
    } else {
      co_await sc.call(&without, knapsack_fn{}, items, idx + 1, cap, val, incumbent);
    }

    co_await sc.join();

    int best = std::max(with, without);
    update_best(*incumbent, best);
    co_return incumbent->load(std::memory_order_relaxed);
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_knapsack(state, threads, [&](knapsack_problem const &problem) -> int {
    std::atomic<int> best{0};
    return lf::schedule(scheduler, knapsack_fn{}, &problem.items, 0, problem.capacity, 0, &best).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, knapsack, knapsack, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, knapsack, knapsack, poly_busy_pool)
