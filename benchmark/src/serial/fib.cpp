#include <benchmark/benchmark.h>

#include "common.hpp"
#include "fib.hpp"
#include "macros.hpp"

import std;

namespace {

auto fib_impl(std::int64_t &ret, std::int64_t n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  fib_impl(lhs, n - 1);
  fib_impl(rhs, n - 2);

  ret = lhs + rhs;
}

template <typename = void>
void fib_serial(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;
    fib_impl(result, n);
    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

auto fib_ret_impl(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  std::int64_t lhs = fib_ret_impl(n - 1);
  std::int64_t rhs = fib_ret_impl(n - 2);

  return lhs + rhs;
}

template <typename = void>
void fib_serial_return(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = fib_ret_impl(n);
    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCH_ALL(fib_serial, serial, fib, fib)
BENCH_ALL(fib_serial_return, serial, fib / return, fib)
