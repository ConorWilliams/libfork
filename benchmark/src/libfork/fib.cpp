#include <benchmark/benchmark.h>

#include "fib.hpp"

#include "helpers.hpp"

import std;

import libfork;

// === Coroutine

namespace {

enum how {
  fork,
  call,
};

template <how A, how B>
struct fib {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t n) -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }

    std::int64_t lhs = 0;
    std::int64_t rhs = 0;

    auto sc = co_await lf::scope();

    if constexpr (A == fork) {
      co_await sc.fork(&lhs, fib{}, n - 2);
    } else {
      co_await sc.call(&lhs, fib{}, n - 2);
    }

    if constexpr (B == fork) {
      co_await sc.fork(&rhs, fib{}, n - 1);
    } else {
      co_await sc.call(&rhs, fib{}, n - 1);
    }

    co_await sc.join();

    co_return lhs + rhs;
  }
};

template <lf::scheduler Sch, how A = fork, how B = call>
void run(benchmark::State &state) {

  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_fib(state, threads, [&](std::int64_t n) -> std::int64_t {
    return lf::schedule(scheduler, fib<A, B>{}, n).get();
  });
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

// -- Multithreaded

LIBFORK_BENCH_ALL_MT(run, fib, fib, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, fib, fib, poly_busy_pool)

// -- Special

using poly_inline = lf::poly_inline_scheduler<geometric_stack<>, adapt_deque<>>;

LIBFORK_BENCH_ALL(run, fib, fib, poly_inline, fork, fork)
LIBFORK_BENCH_ALL(run, fib, fib, poly_inline, fork, call)
LIBFORK_BENCH_ALL(run, fib, fib, poly_inline, call, call)
