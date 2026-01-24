#include <benchmark/benchmark.h>

#include "libfork_benchmark/common.hpp"
#include "libfork_benchmark/fib/fib.hpp"

namespace {

auto fib(std::int64_t &ret, std::int64_t n) -> void {
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

  std::int64_t const n = state.range(0);
  std::int64_t const expect = fib_ref(n);

  state.counters["n"] = n;

  for (auto _ : state) {
    std::int64_t result = 0;
    fib(result, n);
    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCHMARK(fib_serial)->Name("test/serial/fib")->Arg(fib_test);
BENCHMARK(fib_serial)->Name("base/serial/fib")->Arg(fib_base);
