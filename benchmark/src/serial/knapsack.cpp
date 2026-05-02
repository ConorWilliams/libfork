#include <benchmark/benchmark.h>

#include "knapsack.hpp"
#include "macros.hpp"

import std;

namespace {

// Linear-relaxation bound: greedily fill remaining capacity with the densest
// items, taking a fractional piece of the last one.
auto upper_bound(std::vector<knapsack_item> const &items, int idx, int remaining_cap, int current_value) -> double {
  double bound = current_value;
  int cap = remaining_cap;
  for (int i = idx; i < static_cast<int>(items.size()); ++i) {
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

void knapsack_bb(std::vector<knapsack_item> const &items, int idx, int remaining_cap, int current_value, int &best) {

  if (current_value > best) {
    best = current_value;
  }

  if (idx == static_cast<int>(items.size())) {
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

  int n = static_cast<int>(state.range(0));
  auto problem = knapsack_make(n);
  int expect = knapsack_dp_optimum(problem);

  state.counters["n"] = n;
  state.counters["capacity"] = problem.capacity;

  for (auto _ : state) {
    int best = 0;
    benchmark::DoNotOptimize(problem.items.data());
    knapsack_bb(problem.items, 0, problem.capacity, 0, best);
    if (best != expect) {
      state.SkipWithError(std::format("knapsack optimum mismatch: {} != {}", best, expect));
      break;
    }
    benchmark::DoNotOptimize(best);
  }
}

} // namespace

BENCH_ALL(knapsack_serial, serial, knapsack, knapsack)
