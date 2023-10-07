#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "config.hpp"

namespace {

LF_NOINLINE constexpr auto fib(int &ret, int n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  int a, b;

  fib(a, n - 1);
  fib(b, n - 2);

  ret = a + b;
};

void fib_serial(benchmark::State &state) {

  state.counters["green_threads"] = 1;
  state.counters["fib(n)"] = work;

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
    int tmp;
    fib(tmp, secret);
    output = tmp;
  }
}

} // namespace

BENCHMARK(fib_serial)->UseRealTime();
