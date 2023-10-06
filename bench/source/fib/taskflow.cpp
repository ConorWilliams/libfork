#include <benchmark/benchmark.h>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

// https://taskflow.github.io/taskflow/fibonacci.html

auto fib(int n, tf::Subflow &sbf) -> int {

  if (n < 2) {
    return n;
  }

  int res1, res2;

  sbf.emplace([&res1, n](tf::Subflow &sbf) {
    res1 = fib(n - 1, sbf);
  });

  sbf.emplace([&res2, n](tf::Subflow &sbf) {
    res2 = fib(n - 2, sbf);
  });

  sbf.join();

  return res1 + res2;
}

void fib_taskflow(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["fib(n)"] = work;

  std::size_t n = state.range(0);
  tf::Executor executor(n);

  volatile int secret = work;
  volatile int output;

  tf::Taskflow taskflow;

  taskflow.emplace([&output, secret](tf::Subflow &sbf) {
    output = fib(secret, sbf);
  });

  for (auto _ : state) {
    executor.run(taskflow).wait();
  }

#ifndef LF_NO_CHECK
  if (output != sfib(work)) {
    std::cout << "error" << std::endl;
  }
#endif
}

} // namespace

BENCHMARK(fib_taskflow)->Apply(targs)->UseRealTime();
