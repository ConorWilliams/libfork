#pragma once

#include <benchmark/benchmark.h>

#include "common.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

#define BENCH_GET_FN(bench_fn, ...) bench_fn __VA_OPT__(<__VA_ARGS__>)

namespace lf_bench {

inline auto sanitize(std::string s) -> std::string {
  s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
  return s;
}

inline auto
format_name(std::string mode, std::string category, std::string name, std::string args) -> std::string {
  std::string res = sanitize(mode) + "/" + sanitize(category) + "/" + sanitize(name);
  std::string s_args = sanitize(args);
  if (!s_args.empty()) {
    res += "/" + s_args;
  }
  return res;
}

} // namespace lf_bench

// --- Standard Benchmarks ---

#define BENCH_ONE_WITH_ID(id, bench_fn, category, name, mode, prefix, ...)                                   \
  namespace {                                                                                                \
  struct benchmark_reg_##id {                                                                                \
    benchmark_reg_##id() {                                                                                   \
      benchmark::RegisterBenchmark(lf_bench::format_name(#mode, #category, #name, #__VA_ARGS__),             \
                                   BENCH_GET_FN(bench_fn __VA_OPT__(, ) __VA_ARGS__))                        \
          ->Arg(prefix##_##mode)                                                                             \
          ->UseRealTime();                                                                                   \
    }                                                                                                        \
  } benchmark_reg_inst_##id;                                                                                 \
  }

#define BENCH_ONE_HIDDEN(id, ...) BENCH_ONE_WITH_ID(id __VA_OPT__(, ) __VA_ARGS__)
#define BENCH_ONE(bench_fn, category, name, mode, prefix, ...)                                               \
  BENCH_ONE_HIDDEN(__COUNTER__, bench_fn, category, name, mode, prefix __VA_OPT__(, ) __VA_ARGS__)

#define BENCH_ALL(bench_fn, category, name, prefix, ...)                                                     \
  BENCH_ONE(bench_fn, category, name, test, prefix __VA_OPT__(, ) __VA_ARGS__)                               \
  BENCH_ONE(bench_fn, category, name, base, prefix __VA_OPT__(, ) __VA_ARGS__)

// --- Multi-Threaded Benchmarks ---

#define BENCH_ONE_MT_WITH_ID(id, bench_fn, category, name, mode, prefix, ...)                                \
  namespace {                                                                                                \
  struct benchmark_reg_##id {                                                                                \
    benchmark_reg_##id() {                                                                                   \
      benchmark::RegisterBenchmark(lf_bench::format_name(#mode, #category, #name, #__VA_ARGS__),             \
                                   BENCH_GET_FN(bench_fn __VA_OPT__(, ) __VA_ARGS__))                        \
          ->Apply([](benchmark::Benchmark *b) {                                                              \
            bench_thread_args(b, [](benchmark::Benchmark *inner_b, unsigned t) {                             \
              inner_b->Args({prefix##_##mode, static_cast<std::int64_t>(t)});                                \
            });                                                                                              \
          })                                                                                                 \
          ->Complexity([](benchmark::IterationCount n) -> double {                                           \
            return 1.0 / static_cast<double>(n);                                                             \
          })                                                                                                 \
          ->UseRealTime();                                                                                   \
    }                                                                                                        \
  } benchmark_reg_inst_##id;                                                                                 \
  }

#define BENCH_ONE_MT_HIDDEN(id, ...) BENCH_ONE_MT_WITH_ID(id __VA_OPT__(, ) __VA_ARGS__)
#define BENCH_ONE_MT(bench_fn, category, name, mode, prefix, ...)                                            \
  BENCH_ONE_MT_HIDDEN(__COUNTER__, bench_fn, category, name, mode, prefix __VA_OPT__(, ) __VA_ARGS__)

#define BENCH_ALL_MT(bench_fn, category, name, prefix, ...)                                                  \
  BENCH_ONE_MT(bench_fn, category, name, test, prefix __VA_OPT__(, ) __VA_ARGS__)                            \
  BENCH_ONE_MT(bench_fn, category, name, base, prefix __VA_OPT__(, ) __VA_ARGS__)

// --- UTS Benchmarks ---

#define UTS_BENCH_ONE_WITH_ID(id, bench_fn, category, mode, tree_name, tree_id, ...)                         \
  namespace {                                                                                                \
  struct benchmark_reg_##id {                                                                                \
    benchmark_reg_##id() {                                                                                   \
      benchmark::RegisterBenchmark(lf_bench::format_name(#mode, #category, "uts/" tree_name, #__VA_ARGS__),  \
                                   [=](benchmark::State &state) {                                            \
                                     BENCH_GET_FN(bench_fn __VA_OPT__(, ) __VA_ARGS__)(state, tree_id);      \
                                   })                                                                        \
          ->UseRealTime();                                                                                   \
    }                                                                                                        \
  } benchmark_reg_inst_##id;                                                                                 \
  }

#define UTS_BENCH_ONE_HIDDEN(id, ...) UTS_BENCH_ONE_WITH_ID(id __VA_OPT__(, ) __VA_ARGS__)
#define UTS_BENCH_ONE(bench_fn, category, mode, tree_name, tree_id, ...)                                     \
  UTS_BENCH_ONE_HIDDEN(__COUNTER__, bench_fn, category, mode, tree_name, tree_id __VA_OPT__(, ) __VA_ARGS__)

#define UTS_BENCH_ALL(bench_fn, category, ...)                                                               \
  UTS_BENCH_ONE(bench_fn, category, test, "T1_mini", uts_t1_mini __VA_OPT__(, ) __VA_ARGS__)                 \
  UTS_BENCH_ONE(bench_fn, category, test, "T3_mini", uts_t3_mini __VA_OPT__(, ) __VA_ARGS__)                 \
  UTS_BENCH_ONE(bench_fn, category, base, "T1", uts_t1 __VA_OPT__(, ) __VA_ARGS__)                           \
  UTS_BENCH_ONE(bench_fn, category, base, "T3", uts_t3 __VA_OPT__(, ) __VA_ARGS__)                           \
  UTS_BENCH_ONE(bench_fn, category, large, "T1L", uts_t1l __VA_OPT__(, ) __VA_ARGS__)                        \
  UTS_BENCH_ONE(bench_fn, category, large, "T3L", uts_t3l __VA_OPT__(, ) __VA_ARGS__)

// --- UTS Multi-Threaded Benchmarks ---

#define UTS_BENCH_ONE_MT_WITH_ID(id, bench_fn, category, mode, tree_name, tree_id, ...)                      \
  namespace {                                                                                                \
  struct benchmark_reg_##id {                                                                                \
    benchmark_reg_##id() {                                                                                   \
      benchmark::RegisterBenchmark(lf_bench::format_name(#mode, #category, "uts/" tree_name, #__VA_ARGS__),  \
                                   [=](benchmark::State &state) {                                            \
                                     BENCH_GET_FN(bench_fn __VA_OPT__(, ) __VA_ARGS__)(state, tree_id);      \
                                   })                                                                        \
          ->Apply([](benchmark::Benchmark *b) {                                                              \
            bench_thread_args(b, [](benchmark::Benchmark *inner_b, unsigned t) {                             \
              inner_b->Arg(static_cast<std::int64_t>(t));                                                    \
            });                                                                                              \
          })                                                                                                 \
          ->Complexity([](benchmark::IterationCount n) -> double {                                           \
            return 1.0 / static_cast<double>(n);                                                             \
          })                                                                                                 \
          ->UseRealTime();                                                                                   \
    }                                                                                                        \
  } benchmark_reg_inst_##id;                                                                                 \
  }

#define UTS_BENCH_ONE_MT_HIDDEN(id, ...) UTS_BENCH_ONE_MT_WITH_ID(id __VA_OPT__(, ) __VA_ARGS__)
#define UTS_BENCH_ONE_MT(bench_fn, category, mode, tree_name, tree_id, ...)                                  \
  UTS_BENCH_ONE_MT_HIDDEN(                                                                                   \
      __COUNTER__, bench_fn, category, mode, tree_name, tree_id __VA_OPT__(, ) __VA_ARGS__)

#define UTS_BENCH_ALL_MT(bench_fn, category, ...)                                                            \
  UTS_BENCH_ONE_MT(bench_fn, category, test, "T1_mini", uts_t1_mini __VA_OPT__(, ) __VA_ARGS__)              \
  UTS_BENCH_ONE_MT(bench_fn, category, test, "T3_mini", uts_t3_mini __VA_OPT__(, ) __VA_ARGS__)              \
  UTS_BENCH_ONE_MT(bench_fn, category, base, "T1", uts_t1 __VA_OPT__(, ) __VA_ARGS__)                        \
  UTS_BENCH_ONE_MT(bench_fn, category, base, "T3", uts_t3 __VA_OPT__(, ) __VA_ARGS__)                        \
  UTS_BENCH_ONE_MT(bench_fn, category, large, "T1L", uts_t1l __VA_OPT__(, ) __VA_ARGS__)                     \
  UTS_BENCH_ONE_MT(bench_fn, category, large, "T3L", uts_t3l __VA_OPT__(, ) __VA_ARGS__)
