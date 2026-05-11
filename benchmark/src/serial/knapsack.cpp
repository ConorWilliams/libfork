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
      break;
    }
  }
  return bound;
}

void knapsack_bb(std::vector<knapsack_item> const &items,
                 std::size_t idx,
                 int remaining_cap,
                 int current_value,
                 int &best) {

  if (current_value > best) {
    best = current_value;
  }

  if (idx == items.size()) {
    return;
  }

  if (upper_bound(items, idx, remaining_cap, current_value) <= best) {
    return;
  }

  if (items[idx].weight <= remaining_cap) {
    knapsack_bb(items, idx + 1, remaining_cap - items[idx].weight, current_value + items[idx].value, best);
  }
  knapsack_bb(items, idx + 1, remaining_cap, current_value, best);
}

template <typename = void>
void knapsack_serial(benchmark::State &state) {
  run_knapsack(state, [](knapsack_problem const &problem) {
    int best = 0;
    knapsack_bb(problem.items, 0, problem.capacity, 0, best);
    return best;
  });
}

} // namespace

BENCH_ALL(knapsack_serial, serial, knapsack, knapsack)
