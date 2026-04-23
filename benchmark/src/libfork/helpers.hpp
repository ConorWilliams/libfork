
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
