#include <benchmark/benchmark.h>

#include "fib.hpp"

#include "helpers.hpp"

import std;

import libfork;

// === Awaitable and helpers

namespace {

template <lf::scheduler Sch>
struct switch_to_other {

  Sch *target;

  using context_type = Sch::context_type;

  auto await_ready() noexcept -> bool { return false; }

  void await_suspend(lf::sched_handle<context_type> h, context_type &) { target->post(h); }

  auto await_resume() noexcept -> void {}
};

template <lf::scheduler Sch>
struct pool_pair {
  Sch *pools[2];
};

// SplitMix64
struct rng {

  std::uint64_t state;

  auto next() -> rng {
    state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t z = state;
    z = (z ^ (z >> 30ULL)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27ULL)) * 0x94d049bb133111ebULL;
    return {.state = z ^ (z >> 31ULL)};
  }
};

// ~10% switch probability: threshold / 256 ≈ 0.10
inline constexpr std::uint64_t k_switch_threshold = 25;

template <lf::scheduler Sch>
struct random_switch_fib {

  using context_type = Sch::context_type;

  using task = lf::task<std::int64_t, context_type>;

  static auto operator()(std::int64_t n, pool_pair<Sch> *pp, rng seed, unsigned current) -> task {

    if (n < 2) {
      co_return n;
    }

    if ((seed.state & 0xffULL) < k_switch_threshold) {
      current = 1U - current;
      co_await switch_to_other<Sch>{pp->pools[current]};
    }

    std::int64_t lhs = 0;
    std::int64_t rhs = 0;

    auto sc = co_await lf::scope();

    co_await sc.fork(&rhs, random_switch_fib{}, n - 2, pp, seed.next(), current);
    co_await sc.call(&lhs, random_switch_fib{}, n - 1, pp, seed.next(), current);

    co_await sc.join();

    co_return lhs + rhs;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  auto threads_total = static_cast<std::size_t>(state.range(1));

  if (threads_total < 2) {
    state.SkipWithMessage("switch_random requires at least 2 total workers");
    return;
  }

  auto threads_a = (threads_total + 1) / 2;
  auto threads_b = threads_total - threads_a;

  state.counters["n"] = static_cast<double>(n);
  state.counters["p_a"] = static_cast<double>(threads_a);
  state.counters["p_b"] = static_cast<double>(threads_b);

  Sch pool_a{threads_a};
  Sch pool_b{threads_b};

  pool_pair<Sch> pp{&pool_a, &pool_b};

  lf_bench::bench(state, static_cast<std::int64_t>(threads_total), expect, [&]() -> std::int64_t {
    benchmark::DoNotOptimize(n);
    return lf::schedule(pool_a, random_switch_fib<Sch>{}, n, &pp, rng{1}, 0U).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, switch_random, fib, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, switch_random, fib, poly_busy_pool)
