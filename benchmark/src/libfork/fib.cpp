#include <benchmark/benchmark.h>

#include "common.hpp"

#include "fib.hpp"

#include "helpers.hpp"

import std;

import libfork;

// === Coroutine

namespace {

struct fib {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t n) -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }

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

template <typename Stack, template <typename> typename Adaptor>
using real_context = lf::mono_context<Stack, Adaptor>;

template <typename Stack, template <typename> typename Adaptor>
using poly_context = lf::derived_poly_context<Stack, Adaptor>;

using lf::adapt_deque;
using lf::adapt_vector;
using lf::inline_scheduler;

using lf::adaptor_stack;
using lf::geometric_stack;
using lf::slab_stack;

// -- Vector

LIBFORK_BENCH_ALL(run, fib, inline_scheduler<real_context<adaptor_stack<>, adapt_vector>>)
LIBFORK_BENCH_ALL(run, fib, inline_scheduler<poly_context<adaptor_stack<>, adapt_vector>>)

LIBFORK_BENCH_ALL(run, fib, inline_scheduler<real_context<slab_stack<>, adapt_vector>>)
LIBFORK_BENCH_ALL(run, fib, inline_scheduler<poly_context<slab_stack<>, adapt_vector>>)

LIBFORK_BENCH_ALL(run, fib, inline_scheduler<real_context<geometric_stack<>, adapt_vector>>)
LIBFORK_BENCH_ALL(run, fib, inline_scheduler<poly_context<geometric_stack<>, adapt_vector>>)

// -- Deque

LIBFORK_BENCH_ALL(run, fib, inline_scheduler<real_context<adaptor_stack<>, adapt_deque>>)
LIBFORK_BENCH_ALL(run, fib, inline_scheduler<poly_context<adaptor_stack<>, adapt_deque>>)

LIBFORK_BENCH_ALL(run, fib, inline_scheduler<real_context<slab_stack<>, adapt_deque>>)
LIBFORK_BENCH_ALL(run, fib, inline_scheduler<poly_context<slab_stack<>, adapt_deque>>)

LIBFORK_BENCH_ALL(run, fib, inline_scheduler<real_context<geometric_stack<>, adapt_deque>>)
LIBFORK_BENCH_ALL(run, fib, inline_scheduler<poly_context<geometric_stack<>, adapt_deque>>)

LIBFORK_BENCH_ALL_MT(run, fib, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, fib, poly_busy_pool)
