#include <benchmark/benchmark.h>

#include "concurrencpp/concurrencpp.h"

#include "../util.hpp"
#include "config.hpp"

namespace {

using namespace concurrencpp;

auto fibonacci(executor_tag, thread_pool_executor *tpe, const int n) -> result<int> {
  if (n < 2) {
    co_return n;
  }

  auto fib_1 = fibonacci({}, tpe, n - 1);
  auto fib_2 = fibonacci({}, tpe, n - 2);

  co_return co_await fib_1 + co_await fib_2;
}

void fib_ccpp(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["fib(n)"] = work;

  concurrencpp::runtime_options opt;
  opt.max_cpu_threads = state.range(0);
  concurrencpp::runtime runtime(opt);

  auto tpe = runtime.thread_pool_executor();

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
    output = fibonacci({}, tpe.get(), secret).get();
  }

#ifndef LF_NO_CHECK
  if (output != sfib(work)) {
    std::cout << "error" << std::endl;
  }
#endif
}

} // namespace

BENCHMARK(fib_ccpp)->Apply(targs)->UseRealTime();
