#include <benchmark/benchmark.h>

#include <fibril.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

fibril int fib(int n) {
  if (n < 2)
    return n;

  int x, y;
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, &x, fib, (n - 1));

  y = fib(n - 2);
  fibril_join(&fr);

  return x + y;
}

void fib_nowa(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["fib(n)"] = work;

  volatile int secret = work;
  volatile int output;

  fibril_rt_init(state.range(0));

  for (auto _ : state) {
    output = fib(secret);
  }

  fibril_rt_exit();
}

} // namespace

BENCHMARK(fib_nowa)->Apply(targs)->UseRealTime();
