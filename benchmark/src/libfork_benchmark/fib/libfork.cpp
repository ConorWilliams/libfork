#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>

#include <benchmark/benchmark.h>

#include "libfork_benchmark/fib/fib.hpp"

// === Coroutine

namespace {

using lf::env;
using lf::receiver;
using lf::scheduler;
using lf::task;
using lf::worker_context;

struct fib {
  template <worker_context Context>
  static auto operator()(env<Context>, std::int64_t n) -> task<std::int64_t, Context> {
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

template <scheduler Sch>
void run(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  Sch scheduler;

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    receiver recv = lf::schedule2(scheduler, fib{}, n);
    std::int64_t return_value = std::move(recv).get();
    CHECK_RESULT(return_value, expect);
    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

using lf::adapt_vector;
using lf::deque;
using lf::derived_poly_context;
using lf::inline_scheduler;
using lf::mono_context;

using lf::stacks::geometric;

#define BENCH_ONE(mode, ...)                                                                                 \
  BENCHMARK_TEMPLATE(run, __VA_ARGS__)->Name(#mode "/libfork/fib/" #__VA_ARGS__)->Arg(fib_##mode);

#define BENCH_ALL(...) BENCH_ONE(test, __VA_ARGS__) BENCH_ONE(base, __VA_ARGS__)

template <typename Stack, template <typename> typename Adaptor>
using real_context = mono_context<Stack, Adaptor>; // TODO: use container

template <typename Stack, template <typename> typename Adaptor>
using poly_context = derived_poly_context<Stack, Adaptor>; // TODO: use container

BENCH_ALL(inline_scheduler<real_context<geometric<>, adapt_vector>>)

// BENCH_ALL(inline_scheduler<real_context<geometric<>, deque>>)

// BENCH_ALL(inline_scheduler<poly_context<geometric<>>>)
