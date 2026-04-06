#include <benchmark/benchmark.h>

#include "libfork_benchmark/fib/fib.hpp"

import libfork;

import std;

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

    using scope = lf::scope<Context>;

    co_await scope::fork(&rhs, fib{}, n - 2);
    co_await scope::call(&lhs, fib{}, n - 1);

    co_await lf::join();

    co_return lhs + rhs;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  Sch scheduler = [] -> Sch {
    if constexpr (std::constructible_from<Sch, std::size_t>) {
      return Sch{4};
    } else {
      return Sch{};
    }
  }();

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    lf::receiver recv = lf::schedule(scheduler, fib{}, n);
    std::int64_t return_value = std::move(recv).get();
    CHECK_RESULT(return_value, expect);
    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

#define BENCH_ONE(mode, ...)                                                                                 \
  BENCHMARK_TEMPLATE(run, __VA_ARGS__)                                                                       \
      ->Name(#mode "/libfork/fib/" #__VA_ARGS__)                                                             \
      ->Arg(fib_##mode)                                                                                      \
      ->UseRealTime();

#define BENCH_ALL(...) BENCH_ONE(test, __VA_ARGS__) BENCH_ONE(base, __VA_ARGS__)

template <typename Stack, template <typename> typename Adaptor>
using real_context = lf::mono_context<Stack, Adaptor>;

template <typename Stack, template <typename> typename Adaptor>
using poly_context = lf::derived_poly_context<Stack, Adaptor>;

using lf::adapt_deque;
using lf::adapt_vector;
using lf::inline_scheduler;

using lf::adaptor_stack;
using lf::geometric_stack;

BENCH_ALL(inline_scheduler<real_context<adaptor_stack<>, adapt_vector>>)
BENCH_ALL(inline_scheduler<poly_context<adaptor_stack<>, adapt_vector>>)

BENCH_ALL(inline_scheduler<real_context<geometric_stack<>, adapt_vector>>)
BENCH_ALL(inline_scheduler<poly_context<geometric_stack<>, adapt_vector>>)

BENCH_ALL(inline_scheduler<real_context<geometric_stack<>, adapt_deque>>)
BENCH_ALL(inline_scheduler<poly_context<geometric_stack<>, adapt_deque>>)

BENCH_ALL(lf::busy_thread_pool<false, geometric_stack<>>)
BENCH_ALL(lf::busy_thread_pool<true, geometric_stack<>>)
