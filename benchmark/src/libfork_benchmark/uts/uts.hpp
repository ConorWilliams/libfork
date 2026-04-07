#pragma once

// Include the C UTS library header first (it defines max/min macros that would
// clash with std::max/std::min after import std).
#include "libfork_benchmark/uts/external/uts.h"

#undef max
#undef min

import std;

struct result {
  counter_t maxdepth;
  counter_t size;
  counter_t leaves;
  auto operator<=>(const result &) const = default;
};

template <>
struct std::formatter<result> : std::formatter<std::string> {
  auto format(const result &r, auto &ctx) const {
    return std::formatter<std::string>::format(
        std::format("{{maxdepth={}, size={}, leaves={}}}", r.maxdepth, r.size, r.leaves), ctx);
  }
};

struct pair {
  result res;
  Node child;
};

enum uts_tree : int {
  uts_t1_mini, // Geometric [fixed],  ~64K nodes  (test only)
  uts_t1,      // Geometric [fixed],  ~4M nodes
  uts_t1l,     // Geometric [fixed],  ~102M nodes
  uts_t1xxl,   // Geometric [fixed],  ~4.2B nodes
  uts_t3_mini, // Binomial,           ~6K nodes   (test only)
  uts_t3,      // Binomial,           ~4M nodes
  uts_t3l,     // Binomial,           ~111M nodes
  uts_t3xxl,   // Binomial,           ~2.8B nodes
};

// Mini trees used for dry-run correctness checks (fast, ~64K and ~6K nodes).
inline constexpr uts_tree uts_t1_test = uts_t1_mini;
inline constexpr uts_tree uts_t3_test = uts_t3_mini;

void setup_tree(uts_tree tree);

auto expected_result(uts_tree tree) -> result;
