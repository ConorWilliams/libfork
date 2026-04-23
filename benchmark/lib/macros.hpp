#pragma once

#include "common.hpp"
#include <benchmark/benchmark.h>

import std;

// Helper to handle bench_fn with or without template arguments
#define BENCH_GET_FN(bench_fn, ...) bench_fn<__VA_ARGS__>

namespace lf_bench {
inline auto
format_name(std::string mode, std::string category, std::string name, std::string args) -> std::string {
  std::string res = mode + "/" + category + "/" + name;
  if (!args.empty()) {
    res += "/" + args;
  }
  return res;
}
} // namespace lf_bench

// --- Standard Benchmarks (Single Argument for size) ---

#define BENCH_ONE_WITH_ID(id, bench_fn, category, name, mode, prefix, ...)                                   \
  namespace {                                                                                                \
  struct benchmark_reg_##id {                                                                                \
    benchmark_reg_##id() {                                                                                   \
      auto *b = benchmark::RegisterBenchmark(lf_bench::format_name(#mode, #category, #name, #__VA_ARGS__),   \
                                             BENCH_GET_FN(bench_fn, ##__VA_ARGS__));                         \
      b->Arg(prefix##_##mode)->UseRealTime();                                                                \
    }                                                                                                        \
  } benchmark_reg_inst_##id;                                                                                 \
  }

#define BENCH_ONE_HIDDEN(id, ...) BENCH_ONE_WITH_ID(id, ##__VA_ARGS__)
#define BENCH_ONE(bench_fn, category, name, mode, prefix, ...)                                               \
  BENCH_ONE_HIDDEN(__COUNTER__, bench_fn, category, name, mode, prefix, ##__VA_ARGS__)

#define BENCH_ALL(bench_fn, category, name, prefix, ...)                                                     \
  BENCH_ONE(bench_fn, category, name, test, prefix, ##__VA_ARGS__)                                           \
  BENCH_ONE(bench_fn, category, name, base, prefix, ##__VA_ARGS__)

// --- Multi-Threaded Benchmarks (Size and Threads) ---

#define BENCH_ONE_MT_WITH_ID(id, bench_fn, category, name, mode, prefix, ...)                                \
  namespace {                                                                                                \
  struct benchmark_reg_mt_##id {                                                                             \
    benchmark_reg_mt_##id() {                                                                                \
      auto *benchmark_obj =                                                                                  \
          benchmark::RegisterBenchmark(lf_bench::format_name(#mode, #category, #name, #__VA_ARGS__),         \
                                       BENCH_GET_FN(bench_fn, ##__VA_ARGS__));                               \
      benchmark_obj                                                                                          \
          ->Apply([](benchmark::Benchmark *b) -> void {                                                      \
            bench_thread_args(b, [](benchmark::Benchmark *inner_b, unsigned t) {                             \
              inner_b->Args({prefix##_##mode, static_cast<std::int64_t>(t)});                                \
            });                                                                                              \
          })                                                                                                 \
          ->Complexity([](benchmark::IterationCount n) -> double {                                           \
            return 1.0 / static_cast<double>(n);                                                             \
          })                                                                                                 \
          ->UseRealTime();                                                                                   \
    }                                                                                                        \
  } benchmark_reg_mt_inst_##id;                                                                              \
  }

#define BENCH_ONE_MT_HIDDEN(id, ...) BENCH_ONE_MT_WITH_ID(id, ##__VA_ARGS__)
#define BENCH_ONE_MT(bench_fn, category, name, mode, prefix, ...)                                            \
  BENCH_ONE_MT_HIDDEN(__COUNTER__, bench_fn, category, name, mode, prefix, ##__VA_ARGS__)

#define BENCH_ALL_MT(bench_fn, category, name, prefix, ...)                                                  \
  BENCH_ONE_MT(bench_fn, category, name, test, prefix, ##__VA_ARGS__)                                        \
  BENCH_ONE_MT(bench_fn, category, name, base, prefix, ##__VA_ARGS__)

// --- UTS Benchmarks (Tree ID and Threads) ---

#define UTS_BENCH_ONE_WITH_ID(id, bench_fn, category, mode, tree_name, tree_id, ...)                         \
  namespace {                                                                                                \
  struct benchmark_reg_uts_##id {                                                                            \
    benchmark_reg_uts_##id() {                                                                               \
      auto *b = benchmark::RegisterBenchmark(                                                                \
          #mode "/" #category "/uts/" tree_name +                                                            \
              (std::string(#__VA_ARGS__).empty() ? "" : "/" + std::string(#__VA_ARGS__)),                    \
          BENCH_GET_FN(bench_fn, ##__VA_ARGS__));                                                            \
      b->Arg(tree_id)->UseRealTime();                                                                        \
    }                                                                                                        \
  } benchmark_reg_uts_inst_##id;                                                                             \
  }

#define UTS_BENCH_ONE_HIDDEN(id, ...) UTS_BENCH_ONE_WITH_ID(id, ##__VA_ARGS__)
#define UTS_BENCH_ONE(bench_fn, category, mode, tree_name, tree_id, ...)                                     \
  UTS_BENCH_ONE_HIDDEN(__COUNTER__, bench_fn, category, mode, tree_name, tree_id, ##__VA_ARGS__)

#define UTS_BENCH_ONE_MT_WITH_ID(id, bench_fn, category, mode, tree_name, tree_id, ...)                      \
  namespace {                                                                                                \
  struct benchmark_reg_uts_mt_##id {                                                                         \
    benchmark_reg_uts_mt_##id() {                                                                            \
      auto *benchmark_obj = benchmark::RegisterBenchmark(                                                    \
          #mode "/" #category "/uts/" tree_name +                                                            \
              (std::string(#__VA_ARGS__).empty() ? "" : "/" + std::string(#__VA_ARGS__)),                    \
          BENCH_GET_FN(bench_fn, ##__VA_ARGS__));                                                            \
      benchmark_obj                                                                                          \
          ->Apply([](benchmark::Benchmark *b) -> void {                                                      \
            bench_thread_args(b, [](benchmark::Benchmark *inner_b, unsigned t) {                             \
              inner_b->Args({tree_id, static_cast<std::int64_t>(t)});                                        \
            });                                                                                              \
          })                                                                                                 \
          ->Complexity([](benchmark::IterationCount n) -> double {                                           \
            return 1.0 / static_cast<double>(n);                                                             \
          })                                                                                                 \
          ->UseRealTime();                                                                                   \
    }                                                                                                        \
  } benchmark_reg_uts_mt_inst_##id;                                                                          \
  }

#define UTS_BENCH_ONE_MT_HIDDEN(id, ...) UTS_BENCH_ONE_MT_WITH_ID(id, ##__VA_ARGS__)
#define UTS_BENCH_ONE_MT(bench_fn, category, mode, tree_name, tree_id, ...)                                  \
  UTS_BENCH_ONE_MT_HIDDEN(__COUNTER__, bench_fn, category, mode, tree_name, tree_id, ##__VA_ARGS__)
