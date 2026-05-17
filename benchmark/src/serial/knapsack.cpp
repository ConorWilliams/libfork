#include <benchmark/benchmark.h>

#include "knapsack.hpp"
#include "macros.hpp"

import std;

namespace {

// Linear-relaxation bound: greedily fill remaining capacity with the densest
// items, taking a fractional piece of the last one.
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

auto knapsack_bb(std::vector<knapsack_item> const &items, std::size_t idx, int cap, int val, int best) -> int {

  best = std::max(val, best);

  if (idx == items.size()) {
    return best;
  }

  if (upper_bound(items, idx, cap, val) <= best) {
    return best;
  }

  if (items[idx].weight <= cap) {
    best = knapsack_bb(items, idx + 1, cap - items[idx].weight, val + items[idx].value, best);
  }
  return knapsack_bb(items, idx + 1, cap, val, best);
}

template <typename = void>
void knapsack_serial(benchmark::State &state) {
  run_knapsack(state, [](knapsack_problem const &problem) {
    return knapsack_bb(problem.items, 0, problem.capacity, 0, 0);
  });
}

} // namespace

BENCH_ALL(knapsack_serial, serial, knapsack, knapsack)
