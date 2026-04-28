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

  auto await_ready() noexcept -> bool { return false; }
  void await_suspend(lf::sched_handle<typename Sch::context_type> h, typename Sch::context_type &) noexcept {
    target->post(h);
  }
  auto await_resume() noexcept -> void {}
};

template <lf::scheduler Sch>
struct pool_pair {
  Sch *pools[2];
};

// splitmix64 step
constexpr auto next_seed(std::uint64_t seed) noexcept -> std::uint64_t {
  return seed * 6364136223846793005ULL + 0x9E3779B97F4A7C15ULL;
}

// ~10% switch probability: threshold / 256 ≈ 0.10
inline constexpr std::uint64_t k_switch_threshold = 25;

template <lf::scheduler Sch>
struct random_switch_fib {
  using context_type = typename Sch::context_type;

  static auto operator()(lf::env<context_type>, std::int64_t n, pool_pair<Sch> *pp, std::uint64_t seed)
      -> lf::task<std::int64_t, context_type> {
    if (n < 2) {
      co_return n;
    }

    if ((seed & 0xff) < k_switch_threshold) {
      co_await switch_to_other<Sch>{pp->pools[seed & 1]};
    }

    std::int64_t lhs = 0;
    std::int64_t rhs = 0;

    std::uint64_t seed_l = next_seed(seed);
    std::uint64_t seed_r = next_seed(seed_l);

    auto sc = co_await lf::scope();

    co_await sc.fork(&rhs, random_switch_fib{}, n - 2, pp, seed_r);
    co_await sc.call(&lhs, random_switch_fib{}, n - 1, pp, seed_l);

    co_await sc.join();

    co_return lhs + rhs;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  auto threads_total = static_cast<std::size_t>(state.range(1));
  auto threads_a = (threads_total + 1) / 2;
  auto threads_b = threads_total - threads_a;

  if (threads_b == 0) {
    threads_b = 1;
  }

  state.counters["n"] = static_cast<double>(n);
  state.counters["p"] = static_cast<double>(threads_total);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads_total));

  Sch pool_a{threads_a};
  Sch pool_b{threads_b};

  pool_pair<Sch> pp{&pool_a, &pool_b};

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);

    lf::receiver recv = lf::schedule(pool_a, random_switch_fib<Sch>{}, n, &pp, std::uint64_t{1});
    std::int64_t return_value = std::move(recv).get();

    if (return_value != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", return_value, expect));
      break;
    }

    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, switch_random, fib, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, switch_random, fib, poly_busy_pool)
