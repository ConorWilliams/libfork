
#pragma once

#include <benchmark/benchmark.h>

import std;

import libfork;

template <lf::scheduler Sch>
auto thread_count(benchmark::State &state) -> std::size_t {
  if constexpr (std::constructible_from<Sch, std::size_t>) {
    return static_cast<std::size_t>(state.range(1));
  } else {
    return 1;
  }
}

template <lf::scheduler Sch>
auto make_scheduler(benchmark::State &state) -> Sch {
  if constexpr (std::constructible_from<Sch, std::size_t>) {
    return Sch{static_cast<std::size_t>(state.range(1))};
  } else {
    return Sch{};
  }
}

using mono_busy_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_busy_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

#define LIBFORK_BENCH_ONE(bench_fn, name, mode, ...)                                                         \
  BENCHMARK_TEMPLATE(bench_fn, __VA_ARGS__)                                                                  \
      ->Name(#mode "/libfork/" #name "/" #__VA_ARGS__)                                                       \
      ->Arg(name##_##mode)                                                                                   \
      ->UseRealTime();

#define LIBFORK_BENCH_ALL(bench_fn, name, ...)                                                               \
  LIBFORK_BENCH_ONE(bench_fn, name, test, __VA_ARGS__)                                                       \
  LIBFORK_BENCH_ONE(bench_fn, name, base, __VA_ARGS__)

#define LIBFORK_BENCH_ONE_MT(bench_fn, name, mode, ...)                                                      \
  BENCHMARK_TEMPLATE(bench_fn, __VA_ARGS__)                                                                  \
      ->Name(#mode "/libfork/" #name "/" #__VA_ARGS__)                                                       \
      ->Apply([](benchmark::Benchmark *benchmark_obj) -> void {                                              \
        bench_thread_args(benchmark_obj, [](benchmark::Benchmark *b, unsigned t) {                           \
          b->Args({name##_##mode, static_cast<std::int64_t>(t)});                                            \
        });                                                                                                  \
      })                                                                                                     \
      ->Complexity([](benchmark::IterationCount n) -> double {                                               \
        return 1.0 / static_cast<double>(n);                                                                 \
      })                                                                                                     \
      ->UseRealTime();

#define LIBFORK_BENCH_ALL_MT(bench_fn, name, ...)                                                            \
  LIBFORK_BENCH_ONE_MT(bench_fn, name, test, __VA_ARGS__)                                                    \
  LIBFORK_BENCH_ONE_MT(bench_fn, name, base, __VA_ARGS__)

#define OMP_BENCH_ONE(bench_fn, name, mode)                                                                  \
  BENCHMARK_TEMPLATE(bench_fn)                                                                               \
      ->Name(#mode "/openmp/" #name)                                                                         \
      ->Apply([](benchmark::Benchmark *benchmark_obj) -> void {                                              \
        bench_thread_args(benchmark_obj, [](benchmark::Benchmark *b, unsigned t) {                           \
          b->Args({name##_##mode, static_cast<std::int64_t>(t)});                                            \
        });                                                                                                  \
      })                                                                                                     \
      ->Complexity([](benchmark::IterationCount n) -> double {                                               \
        return 1.0 / static_cast<double>(n);                                                                 \
      })                                                                                                     \
      ->UseRealTime();

#define OMP_BENCH_ALL(bench_fn, name)                                                                        \
  OMP_BENCH_ONE(bench_fn, name, test)                                                                        \
  OMP_BENCH_ONE(bench_fn, name, base)

#define LIBFORK_UTS_BENCH_ONE_MT(bench_fn, mode, tree_name, tree_id, ...)                                    \
  BENCHMARK_TEMPLATE(bench_fn, __VA_ARGS__)                                                                  \
      ->Name(#mode "/libfork/uts/" tree_name "/" #__VA_ARGS__)                                               \
      ->Apply([](benchmark::Benchmark *benchmark_obj) -> void {                                              \
        bench_thread_args(benchmark_obj, [](benchmark::Benchmark *b, unsigned t) {                           \
          b->Args({tree_id, static_cast<std::int64_t>(t)});                                                  \
        });                                                                                                  \
      })                                                                                                     \
      ->Complexity([](benchmark::IterationCount n) -> double {                                               \
        return 1.0 / static_cast<double>(n);                                                                 \
      })                                                                                                     \
      ->UseRealTime();

#define OMP_UTS_BENCH_ONE_MT(bench_fn, mode, tree_name, tree_id)                                             \
  BENCHMARK_TEMPLATE(bench_fn)                                                                               \
      ->Name(#mode "/openmp/uts/" tree_name)                                                                 \
      ->Apply([](benchmark::Benchmark *benchmark_obj) -> void {                                              \
        bench_thread_args(benchmark_obj, [](benchmark::Benchmark *b, unsigned t) {                           \
          b->Args({tree_id, static_cast<std::int64_t>(t)});                                                  \
        });                                                                                                  \
      })                                                                                                     \
      ->Complexity([](benchmark::IterationCount n) -> double {                                               \
        return 1.0 / static_cast<double>(n);                                                                 \
      })                                                                                                     \
      ->UseRealTime();
