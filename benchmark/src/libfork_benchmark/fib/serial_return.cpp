#include <benchmark/benchmark.h>

#include "libfork_benchmark/common.hpp"
#include "libfork_benchmark/fib/fib.hpp"

namespace {

auto fib(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  std::int64_t lhs = fib(n - 1);
  std::int64_t rhs = fib(n - 2);

  return lhs + rhs;
}

void fib_serial_return(benchmark::State &state) {

  std::int64_t const n = state.range(0);
  std::int64_t const expect = fib_ref(n);

  state.counters["n"] = n;

  for (auto _ : state) {
    std::int64_t result = fib(n);
    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCHMARK(fib_serial_return)->Name("test/serial/fib/return")->Arg(fib_test);
BENCHMARK(fib_serial_return)->Name("base/serial/fib/return")->Arg(fib_base);
