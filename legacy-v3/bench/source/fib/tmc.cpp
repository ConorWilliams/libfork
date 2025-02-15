#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

#include "tmc/all_headers.hpp"

namespace {

using namespace tmc;

auto fib(int n) -> task<int> {
  if (n < 2) {
    co_return n;
  }

  /* Spawn one, then serially execute the other, then await the first */
  auto xt = spawn(fib(n - 1)).run_early();
  auto y = co_await fib(n - 2);
  auto x = co_await xt;
  co_return x + y;
}

void fib_tmc(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["fib(n)"] = work;

  volatile int secret = work;
  volatile int output;

  tmc::cpu_executor().set_thread_count(state.range(0)).init();

  for (auto _ : state) {
    output = tmc::post_waitable(tmc::cpu_executor(), fib(secret), 0).get();
  }

  tmc::cpu_executor().teardown();

#ifndef LF_NO_CHECK
  if (output != sfib(work)) {
    std::cout << "error" << std::endl;
  }
#endif
}

} // namespace

BENCHMARK(fib_tmc)->Apply(targs)->UseRealTime();