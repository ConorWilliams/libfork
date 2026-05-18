#include <benchmark/benchmark.h>

#include "knapsack.hpp"
#include "macros.hpp"

import std;

namespace {

auto knapsack_bb(std::vector<knapsack_item> const &items, std::size_t idx, int cap, int val, int &best_so_far)
    -> int {
  if (cap < 0) {
    return knapsack_empty;
  }

  if (idx == items.size() || cap == 0) {
    return val;
  }

  if (knapsack_upper_bound(items[idx], cap, val) < best_so_far) {
    return knapsack_empty;
  }

  int with = knapsack_bb(items, idx + 1, cap - items[idx].weight, val + items[idx].value, best_so_far);
  int without = knapsack_bb(items, idx + 1, cap, val, best_so_far);
  int best = std::max(with, without);

  if (best > best_so_far) {
    best_so_far = best;
  }

  return best;
}

template <typename = void>
void knapsack_serial(benchmark::State &state) {
  run_knapsack(state, [](knapsack_problem const &problem) {
    int best_so_far = knapsack_empty;
    return knapsack_bb(problem.items, 0, problem.capacity, 0, best_so_far);
  });
}

} // namespace

BENCH_ALL(knapsack_serial, serial, knapsack, knapsack)
BENCH_ONE(knapsack_serial, serial, knapsack, large, knapsack)
