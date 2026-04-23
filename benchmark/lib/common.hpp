#pragma once

#include <benchmark/benchmark.h>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

inline void bench_thread_args(benchmark::Benchmark *bench, auto make_args) {
  unsigned hw = std::max(1U, std::thread::hardware_concurrency());
  for (unsigned t : {1U, 2U, 4U, 6U, 8U, 12U, 16U, 24U, 32U, 48U, 64U, 96U}) {
    if (t > hw) {
      return;
    }
    make_args(bench, t);
  }
}

template <lf::scheduler Sch>
auto thread_count(benchmark::State &state) -> std::size_t {
  if constexpr (std::constructible_from<Sch, std::size_t>) {
    return static_cast<std::size_t>(state.range(1));
  } else {
    return 1;
  }
}
