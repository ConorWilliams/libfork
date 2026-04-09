#include <benchmark/benchmark.h>

#include "libfork_benchmark/common.hpp"

#include "libfork_benchmark/fib/fib.hpp"

#include "libfork/__impl/compiler.hpp"

import std;

namespace {

LF_NO_INLINE auto fib(std::int64_t &ret, std::int64_t n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  fib(lhs, n - 1);
  fib(rhs, n - 2);

  ret = lhs + rhs;
}

void fib_serial(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;
    fib(result, n);
    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

LF_NO_INLINE auto fib(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  std::int64_t lhs = fib(n - 1);
  std::int64_t rhs = fib(n - 2);

  return lhs + rhs;
}

void fib_serial_return(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = fib(n);
    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCHMARK(fib_serial)->Name("test/serial/fib")->Arg(fib_test);
BENCHMARK(fib_serial)->Name("base/serial/fib")->Arg(fib_base);

BENCHMARK(fib_serial_return)->Name("test/serial/fib/return")->Arg(fib_test);
BENCHMARK(fib_serial_return)->Name("base/serial/fib/return")->Arg(fib_base);
