#include <coroutine>

#include <benchmark/benchmark.h>

#include "libfork_benchmark/common.hpp"
#include "libfork_benchmark/fib/fib.hpp"

import libfork.core;

namespace {

auto fib(std::int64_t *ret, std::int64_t n) -> lf::task<void> {
  if (n < 2) {
    *ret = n;
    co_return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  fib(&lhs, n - 1).release()->handle().resume();
  fib(&rhs, n - 2).release()->handle().resume();

  *ret = lhs + rhs;
}

void fib_alloc(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;

    fib(&result, n).release()->handle().resume();

    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCHMARK(fib_alloc)->Name("test/libfork/fib/alloc")->Arg(fib_test);
BENCHMARK(fib_alloc)->Name("base/libfork/fib/alloc")->Arg(fib_base);
