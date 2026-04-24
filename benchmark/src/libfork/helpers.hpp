#pragma once

#include "macros.hpp"
#include <benchmark/benchmark.h>

#include <concepts>
#include <cstddef>

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

#define LIBFORK_BENCH_ALL(bench_fn, name, prefix, ...)                                                       \
  BENCH_ALL(bench_fn, libfork, name, prefix, ##__VA_ARGS__)

#define LIBFORK_BENCH_ALL_MT(bench_fn, name, prefix, ...)                                                    \
  BENCH_ALL_MT(bench_fn, libfork, name, prefix, ##__VA_ARGS__)

#define LIBFORK_UTS_BENCH_ONE_MT(bench_fn, mode, tree_name, tree_id, ...)                                    \
  UTS_BENCH_ONE_MT(bench_fn, libfork, mode, tree_name, tree_id, ##__VA_ARGS__)

#define LIBFORK_UTS_BENCH_ALL_MT(bench_fn, ...) UTS_BENCH_ALL_MT(bench_fn, libfork, ##__VA_ARGS__)
