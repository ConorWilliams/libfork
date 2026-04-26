#include <benchmark/benchmark.h>

#include "fib.hpp"

#include "helpers.hpp"

import std;

import libfork;

// === Coroutine

namespace {

struct switcher : std::suspend_always {
  template <lf::worker_context Context>
  auto await_suspend(lf::sched_handle<Context> handle, Context &ctx) -> void {
    throw "WAIt for me";
  }
};

struct fib {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t n) -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }

    co_await switcher{};

    std::int64_t lhs = 0;
    std::int64_t rhs = 0;

    auto sc = co_await lf::scope();

    co_await sc.fork(&rhs, fib{}, n - 2);
    co_await sc.call(&lhs, fib{}, n - 1);

    co_await sc.join();

    co_return lhs + rhs;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);
  state.counters["p"] = static_cast<double>(thread_count<Sch>(state));
  state.SetComplexityN(static_cast<benchmark::IterationCount>(thread_count<Sch>(state)));

  Sch scheduler = make_scheduler<Sch>(state);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    lf::receiver recv = lf::schedule(scheduler, fib{}, n);
    std::int64_t return_value = std::move(recv).get();

    if (return_value != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", return_value, expect));
      break;
    }

    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

using lf::adapt_deque;
using lf::adapt_vector;

using lf::adaptor_stack;
using lf::geometric_stack;
using lf::slab_stack;

// -- Vector

LIBFORK_BENCH_ALL(run, fib, fib, lf::mono_inline_scheduler<adaptor_stack<>, adapt_vector<>>)
LIBFORK_BENCH_ALL(run, fib, fib, lf::poly_inline_scheduler<adaptor_stack<>, adapt_vector<>>)

LIBFORK_BENCH_ALL(run, fib, fib, lf::mono_inline_scheduler<slab_stack<>, adapt_vector<>>)
LIBFORK_BENCH_ALL(run, fib, fib, lf::poly_inline_scheduler<slab_stack<>, adapt_vector<>>)

LIBFORK_BENCH_ALL(run, fib, fib, lf::mono_inline_scheduler<geometric_stack<>, adapt_vector<>>)
LIBFORK_BENCH_ALL(run, fib, fib, lf::poly_inline_scheduler<geometric_stack<>, adapt_vector<>>)

// -- Deque

LIBFORK_BENCH_ALL(run, fib, fib, lf::mono_inline_scheduler<adaptor_stack<>, adapt_deque<>>)
LIBFORK_BENCH_ALL(run, fib, fib, lf::poly_inline_scheduler<adaptor_stack<>, adapt_deque<>>)

LIBFORK_BENCH_ALL(run, fib, fib, lf::mono_inline_scheduler<slab_stack<>, adapt_deque<>>)
LIBFORK_BENCH_ALL(run, fib, fib, lf::poly_inline_scheduler<slab_stack<>, adapt_deque<>>)

LIBFORK_BENCH_ALL(run, fib, fib, lf::mono_inline_scheduler<geometric_stack<>, adapt_deque<>>)
LIBFORK_BENCH_ALL(run, fib, fib, lf::poly_inline_scheduler<geometric_stack<>, adapt_deque<>>)

LIBFORK_BENCH_ALL_MT(run, fib, fib, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, fib, fib, poly_busy_pool)
