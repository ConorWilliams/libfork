#include <benchmark/benchmark.h>

#include "helpers.hpp"

import std;

import libfork;

// Constants must be at file scope (outside any namespace) so the macro
// expansion can paste `requests_test` / `requests_base` from any position
// in the translation unit.
inline constexpr std::int64_t requests_test = 64;
inline constexpr std::int64_t requests_base = 1024;

namespace {

inline constexpr std::int64_t k_compute_units = 256;
inline constexpr std::int64_t k_io_units = 32;

inline auto k_io_workers() -> unsigned { return std::max(1u, std::thread::hardware_concurrency() / 8u); }

// Busy-loop work that the optimizer cannot elide.
auto do_work(std::int64_t n) -> std::int64_t {
  std::int64_t acc = 0;
  for (std::int64_t i = 0; i < n; ++i) {
    acc += i ^ (acc >> 1);
    benchmark::DoNotOptimize(acc);
  }
  return acc;
}

// Generic awaitable that posts the task's continuation to an arbitrary pool
// whose context_type matches the current task's Context.
template <typename Sch>
struct switch_to {
  Sch *target;

  auto await_ready() noexcept -> bool { return false; }

  auto await_suspend(lf::sched_handle<typename Sch::context_type> h, typename Sch::context_type &) noexcept
      -> void {
    target->post(h);
  }

  auto await_resume() noexcept -> void {}
};

// One "request": CPU work, hop to IO pool, IO work, hop back, more CPU work.
template <lf::scheduler Sch>
struct request_with_io {
  using context_type = typename Sch::context_type;

  static auto
  operator()(lf::env<context_type>, Sch *compute_pool, Sch *io_pool) -> lf::task<std::int64_t, context_type> {
    std::int64_t acc = do_work(k_compute_units / 2);

    co_await switch_to<Sch>{io_pool};

    acc += do_work(k_io_units);

    co_await switch_to<Sch>{compute_pool};

    acc += do_work(k_compute_units / 2);

    co_return acc;
  }
};

// Baseline: same total work but no pool hops.
template <lf::scheduler Sch>
struct request_baseline {
  using context_type = typename Sch::context_type;

  static auto operator()(lf::env<context_type>) -> lf::task<std::int64_t, context_type> {
    std::int64_t acc = do_work(k_compute_units / 2);
    acc += do_work(k_io_units);
    acc += do_work(k_compute_units / 2);
    co_return acc;
  }
};

// Fan-out: fork M request_with_io tasks and sum the results.
template <lf::scheduler Sch>
struct fan_out_with_io {
  using context_type = typename Sch::context_type;

  static auto operator()(lf::env<context_type>, std::int64_t m, Sch *compute_pool, Sch *io_pool)
      -> lf::task<std::int64_t, context_type> {
    std::vector<std::int64_t> results(static_cast<std::size_t>(m), 0);

    auto sc = co_await lf::scope();

    for (std::int64_t i = 0; i < m - 1; ++i) {
      co_await sc.fork(&results[static_cast<std::size_t>(i)], request_with_io<Sch>{}, compute_pool, io_pool);
    }

    co_await sc.call(
        &results[static_cast<std::size_t>(m - 1)], request_with_io<Sch>{}, compute_pool, io_pool);

    co_await sc.join();

    std::int64_t total = 0;
    for (auto v : results) {
      total += v;
    }
    co_return total;
  }
};

// Fan-out: fork M request_baseline tasks and sum.
template <lf::scheduler Sch>
struct fan_out_baseline {
  using context_type = typename Sch::context_type;

  static auto operator()(lf::env<context_type>, std::int64_t m) -> lf::task<std::int64_t, context_type> {
    std::vector<std::int64_t> results(static_cast<std::size_t>(m), 0);

    auto sc = co_await lf::scope();

    for (std::int64_t i = 0; i < m - 1; ++i) {
      co_await sc.fork(&results[static_cast<std::size_t>(i)], request_baseline<Sch>{});
    }

    co_await sc.call(&results[static_cast<std::size_t>(m - 1)], request_baseline<Sch>{});

    co_await sc.join();

    std::int64_t total = 0;
    for (auto v : results) {
      total += v;
    }
    co_return total;
  }
};

// Compute expected result per request once at startup.
auto expected_per_request() -> std::int64_t {
  return do_work(k_compute_units / 2) + do_work(k_io_units) + do_work(k_compute_units / 2);
}

template <lf::scheduler Sch>
void run_with_io(benchmark::State &state) {
  std::int64_t m = state.range(0);

  state.counters["requests"] = static_cast<double>(m);
  state.counters["compute_threads"] = static_cast<double>(thread_count<Sch>(state));
  state.counters["io_threads"] = static_cast<double>(k_io_workers());
  state.SetComplexityN(static_cast<benchmark::IterationCount>(thread_count<Sch>(state)));

  std::int64_t expect = m * expected_per_request();

  Sch compute_pool = make_scheduler<Sch>(state);
  Sch io_pool{static_cast<std::size_t>(k_io_workers())};

  for (auto _ : state) {
    benchmark::DoNotOptimize(m);
    lf::receiver recv = lf::schedule(compute_pool, fan_out_with_io<Sch>{}, m, &compute_pool, &io_pool);
    std::int64_t result = std::move(recv).get();

    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }

    benchmark::DoNotOptimize(result);
  }
}

template <lf::scheduler Sch>
void run_baseline(benchmark::State &state) {
  std::int64_t m = state.range(0);

  state.counters["requests"] = static_cast<double>(m);
  state.counters["compute_threads"] = static_cast<double>(thread_count<Sch>(state));
  state.SetComplexityN(static_cast<benchmark::IterationCount>(thread_count<Sch>(state)));

  std::int64_t expect = m * expected_per_request();

  Sch compute_pool = make_scheduler<Sch>(state);

  for (auto _ : state) {
    benchmark::DoNotOptimize(m);
    lf::receiver recv = lf::schedule(compute_pool, fan_out_baseline<Sch>{}, m);
    std::int64_t result = std::move(recv).get();

    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }

    benchmark::DoNotOptimize(result);
  }
}

} // namespace

// prefix = requests  →  macro uses requests_test / requests_base
LIBFORK_BENCH_ALL_MT(run_with_io, request_io, requests, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run_baseline, request_baseline, requests, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run_with_io, request_io, requests, poly_busy_pool)
LIBFORK_BENCH_ALL_MT(run_baseline, request_baseline, requests, poly_busy_pool)
