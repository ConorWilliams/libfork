#pragma once

#include <benchmark/benchmark.h>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr int fib_test = 8;
inline constexpr int fib_base = 37;

/**
 * @brief Non-recursive Fibonacci calculation
 */
constexpr auto fib_ref(std::int64_t n) -> std::int64_t {

  if (n < 2) {
    return n;
  }

  std::int64_t prev = 0;
  std::int64_t curr = 1;

  for (std::int64_t i = 2; i <= n; ++i) {
    std::int64_t next = prev + curr;
    prev = curr;
    curr = next;
  }

  return curr;
}

template <typename Fn>
void run_fib(benchmark::State &state, std::int64_t threads, Fn fn) {
  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  lf_bench::bench(state, threads, expect, [n, fn]() -> std::int64_t {
    return std::invoke(fn, n);
  });
}

template <typename Fn>
void run_fib(benchmark::State &state, Fn fn) {
  run_fib(state, lf_bench::no_threads, fn);
}
