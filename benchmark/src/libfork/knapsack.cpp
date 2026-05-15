#include <benchmark/benchmark.h>

#include "knapsack.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr std::size_t knapsack_parallel_depth = 10;

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
      break;
    }
  }
  return bound;
}

void update_best(std::atomic<int> &best, int value) {
  int old = best.load(std::memory_order_relaxed);
  while (value > old && !best.compare_exchange_weak(old, value, std::memory_order_relaxed)) {
  }
}

void knapsack_serial_tail(std::vector<knapsack_item> const &items,
                          std::size_t idx,
                          int remaining_cap,
                          int current_value,
                          std::atomic<int> &best) {

  update_best(best, current_value);

  if (idx == items.size()) {
    return;
  }

  if (upper_bound(items, idx, remaining_cap, current_value) <= best.load(std::memory_order_relaxed)) {
    return;
  }

  if (items[idx].weight <= remaining_cap) {
    knapsack_serial_tail(items,
                         idx + 1,
                         remaining_cap - items[idx].weight,
                         current_value + items[idx].value,
                         best);
  }
  knapsack_serial_tail(items, idx + 1, remaining_cap, current_value, best);
}

struct knapsack_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         std::vector<knapsack_item> const *items,
                         std::size_t idx,
                         int remaining_cap,
                         int current_value,
                         std::atomic<int> *best) -> lf::task<int, Context> {

    update_best(*best, current_value);

    if (idx == items->size()) {
      co_return current_value;
    }

    if (upper_bound(*items, idx, remaining_cap, current_value) <= best->load(std::memory_order_relaxed)) {
      co_return best->load(std::memory_order_relaxed);
    }

    if (idx >= knapsack_parallel_depth) {
      knapsack_serial_tail(*items, idx, remaining_cap, current_value, *best);
      co_return best->load(std::memory_order_relaxed);
    }

    int take = current_value;
    int skip = current_value;

    auto sc = co_await lf::scope();
    if ((*items)[idx].weight <= remaining_cap) {
      co_await sc.fork(&take,
                       knapsack_task{},
                       items,
                       idx + 1,
                       remaining_cap - (*items)[idx].weight,
                       current_value + (*items)[idx].value,
                       best);
    }
    co_await sc.call(&skip, knapsack_task{}, items, idx + 1, remaining_cap, current_value, best);
    co_await sc.join();

    co_return std::max({take, skip, best->load(std::memory_order_relaxed)});
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_knapsack(state, [&](knapsack_problem const &problem) {
    std::atomic<int> best{0};
    return lf::schedule(scheduler, knapsack_task{}, &problem.items, 0UZ, problem.capacity, 0, &best).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, knapsack, knapsack, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, knapsack, knapsack, poly_busy_pool)
