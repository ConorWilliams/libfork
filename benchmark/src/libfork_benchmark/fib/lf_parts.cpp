#include <coroutine>

#include <benchmark/benchmark.h>

#include "libfork/macros.hpp"

#include "libfork_benchmark/common.hpp"
#include "libfork_benchmark/fib/fib.hpp"

import libfork.core;

namespace {

auto fib(std::int64_t &ret, std::int64_t n) -> lf::task<void> {
  if (n < 2) {
    ret = n;
    co_return;
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

} // namespace

BENCHMARK(fib_serial)->Name("test/serial/fib")->Arg(fib_test);
BENCHMARK(fib_serial)->Name("base/serial/fib")->Arg(fib_base);
