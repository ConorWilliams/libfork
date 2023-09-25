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

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
    output = arena.execute([&] {
      return fib(secret);
    });
  }
}

} // namespace

BENCHMARK(fib_tbb)->DenseRange(1, num_threads())->UseRealTime();
