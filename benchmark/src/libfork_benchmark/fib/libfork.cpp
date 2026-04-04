#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>

#include <benchmark/benchmark.h>

#include "libfork_benchmark/fib/fib.hpp"

// === Coroutine

namespace  {

using lf::env;
using lf::task;
using lf::worker_context;

constexpr auto fib = []<worker_context Context>(this auto fib, env<Context>, std::int64_t n) -> task<std::int64_t, Context> {

  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  using scope = lf::scope<Context>;

  co_await scope::fork(&rhs, fib, n - 2);
  co_await scope::call(&lhs, fib, n - 1);

  co_await lf::join();

  co_return lhs + rhs;
};

template <worker_context Context>
void run(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  Context context;

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t return_value = 0;
    lf::schedule(&context, fib, n);
    CHECK_RESULT(return_value, expect);
    benchmark::DoNotOptimize(return_value);
  }
}

}

using lf::inline_context;
using lf::deque;
using lf::stack::geometric;

// Minimal coroutine, bump allocated (thread-local) stack
BENCHMARK_TEMPLATE(run, inline_context<false, geometric<>>)->Name("test/libfork/inline/nopoly/geometric")->Arg(fib_test);
BENCHMARK_TEMPLATE(run, inline_context<false, geometric<>>)->Name("base/libfork/inline/nopoly/geometric")->Arg(fib_base);

