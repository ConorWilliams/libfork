#include <thread>
#include <vector>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto fib(int n) -> int {

  if (n < 2) {
    return n;
  }

  int x, y;

#pragma omp task untied shared(x) firstprivate(n) default(none)
  x = fib(n - 1);

  y = fib(n - 2);

#pragma omp taskwait

  return x + y;
}

void fib_omp(benchmark::State &state) {

  std::size_t n = state.range(0);

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
#pragma omp parallel num_threads(n)
#pragma omp single
    output = fib(secret);
  }

  if (output != sfib(work)) {
    std::cout << "error" << std::endl;
  }
}

} // namespace

BENCHMARK(fib_omp)->DenseRange(1, num_threads())->UseRealTime();
