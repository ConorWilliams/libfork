#include <benchmark/benchmark.h>

namespace {

auto fib(int &ret, int n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  int a = 0;
  int b = 0;

  fib(a, n - 1);
  fib(b, n - 2);

  ret = a + b;
}

void fib_serial(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));

  state.counters["threads"] = 1;
  state.counters["n"] = n;

  for (auto _ : state) {
    int result = 0;
    fib(result, n);
    benchmark::DoNotOptimize(result);
  }

  state.SetComplexityN(n);
}

} // namespace

BENCHMARK(fib_serial)->Arg(10)->Arg(20)->Arg(30)->Arg(40)->Complexity()->UseRealTime();
