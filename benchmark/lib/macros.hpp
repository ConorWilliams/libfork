#pragma once

#include <benchmark/benchmark.h>
#include "common.hpp"

// --- Standard Benchmarks (Single Argument for size) ---

#define BENCH_ONE(bench_fn, category, name, mode, ...)                                                       \
  BENCHMARK_TEMPLATE(bench_fn, __VA_ARGS__)                                                                  \
      ->Name(#mode "/" #category "/" #name "/" #__VA_ARGS__)                                                 \
      ->Arg(name##_##mode)                                                                                   \
      ->UseRealTime();

#define BENCH_ALL(bench_fn, category, name, ...)                                                             \
  BENCH_ONE(bench_fn, category, name, test, __VA_ARGS__)                                                     \
  BENCH_ONE(bench_fn, category, name, base, __VA_ARGS__)

// --- Multi-Threaded Benchmarks (Size and Threads) ---

#define BENCH_ONE_MT(bench_fn, category, name, mode, ...)                                                    \
  BENCHMARK_TEMPLATE(bench_fn, __VA_ARGS__)                                                                  \
      ->Name(#mode "/" #category "/" #name "/" #__VA_ARGS__)                                                 \
      ->Apply([](benchmark::Benchmark *benchmark_obj) -> void {                                              \
        bench_thread_args(benchmark_obj, [](benchmark::Benchmark *b, unsigned t) {                           \
          b->Args({name##_##mode, static_cast<std::int64_t>(t)});                                            \
        });                                                                                                  \
      })                                                                                                     \
      ->Complexity([](benchmark::IterationCount n) -> double {                                               \
        return 1.0 / static_cast<double>(n);                                                                 \
      })                                                                                                     \
      ->UseRealTime();

#define BENCH_ALL_MT(bench_fn, category, name, ...)                                                          \
  BENCH_ONE_MT(bench_fn, category, name, test, __VA_ARGS__)                                                  \
  BENCH_ONE_MT(bench_fn, category, name, base, __VA_ARGS__)

// --- UTS Benchmarks (Tree ID and Threads) ---

#define UTS_BENCH_ONE_MT(bench_fn, category, mode, tree_name, tree_id, ...)                                  \
  BENCHMARK_TEMPLATE(bench_fn, __VA_ARGS__)                                                                  \
      ->Name(#mode "/" #category "/uts/" tree_name "/" #__VA_ARGS__)                                         \
      ->Apply([](benchmark::Benchmark *benchmark_obj) -> void {                                              \
        bench_thread_args(benchmark_obj, [](benchmark::Benchmark *b, unsigned t) {                           \
          b->Args({tree_id, static_cast<std::int64_t>(t)});                                                  \
        });                                                                                                  \
      })                                                                                                     \
      ->Complexity([](benchmark::IterationCount n) -> double {                                               \
        return 1.0 / static_cast<double>(n);                                                                 \
      })                                                                                                     \
      ->UseRealTime();
