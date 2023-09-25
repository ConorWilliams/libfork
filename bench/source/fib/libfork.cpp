#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "config.hpp"

namespace {

inline constexpr lf::async fib = [](auto fib, int n) LF_STATIC_CALL -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, fib)(n - 1);
  co_await lf::call(b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

void fib_libfork(benchmark::State &state) {

  lf::lazy_pool sch(state.range(0));

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
    output = lf::sync_wait(sch, fib, secret);
  }
}

} // namespace

BENCHMARK(fib_libfork)->DenseRange(1, std::thread::hardware_concurrency())->UseRealTime();
