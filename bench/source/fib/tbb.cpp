#include <benchmark/benchmark.h>

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

  tbb::task_group g;

  g.run([&] {
    a = fib(n - 1);
  });

  b = fib(n - 2);

  g.wait();

  return a + b;
}

void fib_tbb(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["fib(n)"] = work;

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
    output = arena.execute([&] {
      return fib(secret);
    });
  }

#ifndef LF_NO_CHECK
  if (output != sfib(work)) {
    std::cout << "error" << std::endl;
  }
#endif
}

} // namespace

BENCHMARK(fib_tbb)->Apply(targs)->UseRealTime();
