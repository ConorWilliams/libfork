#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
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

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void fib_libfork(benchmark::State &state) {

  Sch sch(state.range(0));

  volatile int secret = work;
  volatile int output;

  for (auto _ : state) {
    output = lf::sync_wait(sch, fib, secret);
  }
}

} // namespace

using namespace lf;

BENCHMARK(fib_libfork<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(fib_libfork<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(fib_libfork<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(fib_libfork<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();