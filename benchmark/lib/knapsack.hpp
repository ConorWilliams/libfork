#pragma once

#include <algorithm>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <limits>
  #include <vector>
#else
import std;
#endif

inline constexpr std::int64_t knapsack_test = 500;
inline constexpr std::int64_t knapsack_base = 900;
inline constexpr std::int64_t knapsack_large = 1'240;
inline constexpr std::size_t knapsack_item_count = 32;
inline constexpr int knapsack_empty = std::numeric_limits<int>::min();

struct knapsack_item {
  int value;
  int weight;
};

struct knapsack_problem {
  std::vector<knapsack_item> items; // sorted by value/weight desc
  int capacity;
};

inline constexpr std::array<knapsack_item, knapsack_item_count> knapsack_items = {{
    {.value = 15, .weight = 23}, {.value = 22, .weight = 12}, {.value = 17, .weight = 42},
    {.value = 1, .weight = 13},  {.value = 32, .weight = 21}, {.value = 65, .weight = 43},
    {.value = 23, .weight = 56}, {.value = 4, .weight = 7},   {.value = 4, .weight = 8},
    {.value = 32, .weight = 42}, {.value = 51, .weight = 32}, {.value = 22, .weight = 12},
    {.value = 17, .weight = 24}, {.value = 12, .weight = 13}, {.value = 23, .weight = 21},
    {.value = 56, .weight = 47}, {.value = 23, .weight = 65}, {.value = 6, .weight = 7},
    {.value = 4, .weight = 7},   {.value = 32, .weight = 42}, {.value = 22, .weight = 42},
    {.value = 59, .weight = 32}, {.value = 23, .weight = 12}, {.value = 12, .weight = 24},
    {.value = 12, .weight = 13}, {.value = 23, .weight = 21}, {.value = 39, .weight = 48},
    {.value = 22, .weight = 65}, {.value = 6, .weight = 7},   {.value = 4, .weight = 7},
    {.value = 33, .weight = 42}, {.value = 18, .weight = 53},
}};

inline auto knapsack_density_less(knapsack_item a, knapsack_item b) -> bool {
  return static_cast<double>(a.value) / a.weight > static_cast<double>(b.value) / b.weight;
}

inline auto knapsack_make(int capacity) -> knapsack_problem {
  std::vector<knapsack_item> items;
  int total_weight = 0;
  for (std::size_t i = 0; total_weight <= capacity; ++i) {
    items.push_back(knapsack_items[i % knapsack_items.size()]);
    total_weight += items.back().weight;
  }

  // Match nowa: sort on decreasing value/weight before the search.
  std::sort(items.begin(), items.end(), knapsack_density_less);

  return knapsack_problem{.items = std::move(items), .capacity = capacity};
}

// Exact optimum via O(n * capacity) DP, used as oracle.
inline auto knapsack_dp_optimum(knapsack_problem const &p) -> int {

  std::vector<int> dp(static_cast<std::size_t>(p.capacity) + 1, 0);

  for (auto const &it : p.items) {
    for (int c = p.capacity; c >= it.weight; --c) {
      auto idx = static_cast<std::size_t>(c);
      auto idx_prev = static_cast<std::size_t>(c - it.weight);
      int cand = dp[idx_prev] + it.value;
      dp[idx] = std::max(cand, dp[idx]);
    }
  }

  return dp[static_cast<std::size_t>(p.capacity)];
}

inline auto knapsack_upper_bound(knapsack_item const &item, int cap, int val) -> double {
  return static_cast<double>(val) + static_cast<double>(cap) * item.value / item.weight;
}

template <typename Fn>
void run_knapsack(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto capacity = static_cast<int>(state.range(0));
  auto problem = knapsack_make(capacity);
  int expect = knapsack_dp_optimum(problem);

  state.counters["n"] = static_cast<double>(problem.items.size());
  state.counters["capacity"] = problem.capacity;

  lf_bench::bench(state, threads, expect, [problem = std::move(problem), fn]() -> int {
    return std::invoke(fn, problem);
  });
}

template <typename Fn>
void run_knapsack(benchmark::State &state, Fn fn) {
  run_knapsack(state, lf_bench::no_threads, fn);
}
