#pragma once

// Include the C UTS library header first (it defines max/min macros that would
// clash with std::max/std::min after import std).
#include "libfork_benchmark/uts/external/uts.h"

#undef max
#undef min

#include "libfork_benchmark/common.hpp"

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
  uts_t1_mini = 10, // Geometric [fixed],  ~21K nodes  (test only)
  uts_t1 = 11,      // Geometric [fixed],  ~4M nodes
  uts_t1l = 12,     // Geometric [fixed],  ~102M nodes
  uts_t1xxl = 13,   // Geometric [fixed],  ~4.2B nodes
  uts_t3_mini = 30, // Binomial,           ~41K nodes  (test only)
  uts_t3 = 31,      // Binomial,           ~4M nodes
  uts_t3l = 32,     // Binomial,           ~111M nodes
  uts_t3xxl = 33,   // Binomial,           ~2.8B nodes
};

// Mini trees used for dry-run correctness checks (fast, ~21K and ~41K nodes).
inline constexpr uts_tree uts_t1_test = uts_t1_mini;
inline constexpr uts_tree uts_t3_test = uts_t3_mini;

// ---- Tree setup ----

inline void reset_uts() {
  type = GEO;
  b_0 = 4.0;
  rootId = 0;
  nonLeafBF = 4;
  nonLeafProb = 15.0 / 64.0;
  gen_mx = 6;
  shape_fn = LINEAR;
  shiftDepth = 0.5;
  computeGranularity = 1;
  debug = 0;
  verbose = 1;
}

// (T1 mini) Geometric [fixed] — gen_mx=7, same other params as T1
inline void setup_t1_mini() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 7;
  b_0 = 4;
  rootId = 19;
}

// (T1) Geometric [fixed] — size = 4130071, depth = 10, leaves = 3305118
inline void setup_t1() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 10;
  b_0 = 4;
  rootId = 19;
}

// (T1L) Geometric [fixed] — size = 102181082, depth = 13, leaves = 81746377
inline void setup_t1l() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 13;
  b_0 = 4;
  rootId = 29;
}

// (T1XXL) Geometric [fixed] — size = 4230646601, depth = 15
inline void setup_t1xxl() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 15;
  b_0 = 4;
  rootId = 19;
}

// (T3 mini) Binomial — b_0=20, same other params as T3
inline void setup_t3_mini() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 20;
  nonLeafBF = 8;
  nonLeafProb = 0.124875;
  rootId = 42;
}

// (T3) Binomial — size = 4112897, depth = 1572, leaves = 3599034
inline void setup_t3() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 2000;
  nonLeafBF = 8;
  nonLeafProb = 0.124875;
  rootId = 42;
}

// (T3L) Binomial — size = 111345631, depth = 17844, leaves = 89076904
inline void setup_t3l() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 2000;
  nonLeafBF = 5;
  nonLeafProb = 0.200014;
  rootId = 7;
}

// (T3XXL) Binomial — size = 2793220501
inline void setup_t3xxl() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 2000;
  nonLeafBF = 2;
  nonLeafProb = 0.499995;
  rootId = 316;
}

inline void setup_tree(uts_tree tree) {
  switch (tree) {
    case uts_t1_mini:
      setup_t1_mini();
      break;
    case uts_t1:
      setup_t1();
      break;
    case uts_t1l:
      setup_t1l();
      break;
    case uts_t1xxl:
      setup_t1xxl();
      break;
    case uts_t3_mini:
      setup_t3_mini();
      break;
    case uts_t3:
      setup_t3();
      break;
    case uts_t3l:
      setup_t3l();
      break;
    case uts_t3xxl:
      setup_t3xxl();
      break;
  }
}

inline auto expected_result(uts_tree tree) -> result {
  switch (tree) {
    case uts_t1_mini:
      return {7, 63914, 51124};
    case uts_t1:
      return {10, 4130071, 3305118};
    case uts_t1l:
      return {13, 102181082, 81746377};
    case uts_t1xxl:
      return {15, 4230646601, 3384495738};
    case uts_t3_mini:
      return {67, 6213, 5438};
    case uts_t3:
      return {1572, 4112897, 3599034};
    case uts_t3l:
      return {17844, 111345631, 89076904};
    case uts_t3xxl:
      return {99049, 2793220501, 1396611250};
  }
  std::unreachable();
}
