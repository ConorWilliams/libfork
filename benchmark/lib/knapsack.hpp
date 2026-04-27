#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <cstddef>
  #include <cstdint>
  #include <random>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t knapsack_test = 16;
inline constexpr std::size_t knapsack_base = 28;

struct knapsack_item {
  int weight;
  int value;
};

struct knapsack_problem {
  std::vector<knapsack_item> items; // sorted by value/weight desc
  int capacity;
};

inline auto knapsack_make(std::size_t n, std::uint64_t seed = 0xCAFEBABE) -> knapsack_problem {
  std::mt19937_64 rng{seed};
  std::uniform_int_distribution<int> dw(1, 100);
  std::uniform_int_distribution<int> dv(1, 100);

  std::vector<knapsack_item> items(n);
  int total = 0;
  for (auto &it : items) {
    it.weight = dw(rng);
    it.value = dv(rng);
    total += it.weight;
  }

  // Sort by value-density, descending, for a tight relaxation bound.
  std::sort(items.begin(), items.end(), [](knapsack_item a, knapsack_item b) {
    return static_cast<long long>(a.value) * b.weight > static_cast<long long>(b.value) * a.weight;
  });

  return knapsack_problem{std::move(items), total / 2};
}

// Exact optimum via O(n * capacity) DP, used as oracle.
inline auto knapsack_dp_optimum(knapsack_problem const &p) -> int {
  std::vector<int> dp(static_cast<std::size_t>(p.capacity) + 1, 0);
  for (auto const &it : p.items) {
    for (int c = p.capacity; c >= it.weight; --c) {
      auto idx = static_cast<std::size_t>(c);
      auto idx_prev = static_cast<std::size_t>(c - it.weight);
      int cand = dp[idx_prev] + it.value;
      if (cand > dp[idx]) {
        dp[idx] = cand;
      }
    }
  }
  return dp[static_cast<std::size_t>(p.capacity)];
}
