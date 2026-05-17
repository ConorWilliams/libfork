#pragma once

#include <algorithm>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <random>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t knapsack_test = 16;
inline constexpr std::size_t knapsack_base = 88;

struct knapsack_item {
  int weight;
  int value;
};

struct knapsack_problem {
  std::vector<knapsack_item> items; // sorted by value/weight desc
  int capacity;
};

inline auto knapsack_make(std::size_t n, std::uint64_t seed = 144) -> knapsack_problem {
  std::mt19937_64 rng{seed};
  std::uniform_int_distribution<int> noise(-50, 50);

  std::vector<knapsack_item> items(n);
  int total = 0;
  for (auto &it : items) {
    it.weight = 10'000 + noise(rng);
    it.value = it.weight + noise(rng);
    total += it.weight;
  }

  // Sort by value-density, descending, for a tight relaxation bound.
  std::sort(items.begin(), items.end(), [](knapsack_item a, knapsack_item b) {
    return static_cast<long long>(a.value) * b.weight > static_cast<long long>(b.value) * a.weight;
  });

  return knapsack_problem{.items = std::move(items), .capacity = total / 2};
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

template <typename Fn>
void run_knapsack(benchmark::State &state, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  auto problem = knapsack_make(n);
  int expect = knapsack_dp_optimum(problem);

  state.counters["n"] = static_cast<double>(n);
  state.counters["capacity"] = problem.capacity;

  lf_bench::bench(state, expect, [problem = std::move(problem), fn]() -> int {
    return std::invoke(fn, problem);
  });
}
