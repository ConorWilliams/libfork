#pragma once

#include <benchmark/benchmark.h>

#include "bench.hpp"

// Include the C UTS library header first (it defines max/min macros that would
// clash with std::max/std::min after import std).
#include "uts/uts.h"

#undef max
#undef min

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
  #include <format>
  #include <string>
#else
import std;
#endif

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

enum uts_tree : char {
  uts_t1_mini, // Geometric [fixed],  ~64K nodes  (test only)
  uts_t1,      // Geometric [fixed],  ~4M nodes
  uts_t1l,     // Geometric [fixed],  ~102M nodes
  uts_t1xxl,   // Geometric [fixed],  ~4.2B nodes
  uts_t3_mini, // Binomial,           ~6K nodes   (test only)
  uts_t3,      // Binomial,           ~4M nodes
  uts_t3l,     // Binomial,           ~111M nodes
  uts_t3xxl,   // Binomial,           ~2.8B nodes
};

void setup_tree(uts_tree tree);

auto expected_result(uts_tree tree) -> result;

template <typename Fn>
void run_uts(benchmark::State &state, uts_tree tree, std::int64_t threads, Fn fn) {
  setup_tree(tree);
  auto expect = expected_result(tree);
  result stats{};

  lf_bench::bench(state, threads, expect, [&stats, fn]() -> result {
    Node root;
    uts_initRoot(&root, type);
    return stats = std::invoke(fn, &root);
  });

  state.counters["depth"] = static_cast<double>(stats.maxdepth);
  state.counters["nodes"] = static_cast<double>(stats.size);
  state.counters["leaves"] = static_cast<double>(stats.leaves);
}

template <typename Fn>
void run_uts(benchmark::State &state, uts_tree tree, Fn fn) {
  run_uts(state, tree, lf_bench::no_threads, fn);
}
