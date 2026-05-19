#include <benchmark/benchmark.h>

#include "helpers.hpp"

import std;

import libfork;

inline constexpr std::int64_t requests_test = 64;
inline constexpr std::int64_t requests_base = (1 << 16) - 2;

namespace {

inline constexpr std::int64_t k_compute_units = 256;
inline constexpr std::int64_t k_io_units = 32;

inline auto k_io_workers() -> unsigned { return std::max(2u, std::thread::hardware_concurrency() / 8u); }

// Busy-loop work that the optimizer cannot elide.
auto do_work(std::int64_t n) -> std::int64_t {
  std::int64_t acc = 0;
  for (std::int64_t i = 0; i < n; ++i) {
    acc += i ^ (acc >> 1);
  }
  return acc;
}

template <typename Sch>
struct switch_to {

  using context_type = Sch::context_type;

  Sch *target;

  auto await_ready() noexcept -> bool { return false; }

  auto await_suspend(lf::sched_handle<context_type> h, context_type & /*context*/) -> void {
    target->post(h);
  }

  auto await_resume() noexcept -> void {}
};

template <lf::scheduler Sch>
struct request_with_io {

  using context_type = Sch::context_type;

  Sch *compute_pool;
  Sch *io_pool;

  auto operator()(this auto self, int) -> lf::task<std::int64_t, context_type> {

    std::int64_t acc = do_work(k_compute_units / 2);

    co_await switch_to<Sch>{self.io_pool};

    acc += do_work(k_io_units);

    co_await switch_to<Sch>{self.compute_pool};

    acc += do_work(k_compute_units / 2);

    co_return acc;
  }
};

// Baseline: same total work but no pool hops.
template <lf::scheduler Sch>
struct request_baseline {

  using context_type = Sch::context_type;

  static auto operator()(int) -> lf::task<std::int64_t, context_type> {
    std::int64_t acc = do_work(k_compute_units / 2);
    acc += do_work(k_io_units);
    acc += do_work(k_compute_units / 2);
    co_return acc;
  }
};

// Fan-out: fork M request_with_io tasks and sum the results.
template <lf::scheduler Sch>
struct fan_out_with_io {

  using context_type = Sch::context_type;

  static auto
  operator()(std::int64_t m, Sch *compute_pool, Sch *io_pool) -> lf::task<std::int64_t, context_type> {

    auto inputs = std::ranges::views::repeat(int{0}, m);
    auto fn = request_with_io<Sch>{compute_pool, io_pool};
    std::optional<std::int64_t> total;

    auto sc = co_await lf::scope();
    co_await sc.call(&total, lf::fold, inputs, std::plus<>{}, fn);
    co_await sc.join();

    co_return total.value_or(0);
  }
};

// Fan-out: fork M request_baseline tasks and sum.
template <lf::scheduler Sch>
struct fan_out_baseline {

  using context_type = Sch::context_type;

  static auto operator()(std::int64_t m) -> lf::task<std::int64_t, context_type> {

    auto inputs = std::ranges::views::repeat(int{0}, m);
    auto fn = request_baseline<Sch>{};

    std::optional<std::int64_t> total;
    auto sc = co_await lf::scope();
    co_await sc.call(&total, lf::fold, inputs, std::plus<>{}, fn);
    co_await sc.join();

    co_return total.value_or(0);
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
  state.counters["compute_thr"] = static_cast<double>(thread_count<Sch>(state));
  state.counters["io_thr"] = static_cast<double>(k_io_workers());

  std::int64_t expect = m * expected_per_request();

  Sch compute_pool = make_scheduler<Sch>(state);
  Sch io_pool{static_cast<std::size_t>(k_io_workers())};

  lf_bench::bench(state, static_cast<std::int64_t>(thread_count<Sch>(state)), expect, [&]() -> std::int64_t {
    return lf::schedule(compute_pool, fan_out_with_io<Sch>{}, m, &compute_pool, &io_pool).get();
  });
}

template <lf::scheduler Sch>
void run_baseline(benchmark::State &state) {
  std::int64_t m = state.range(0);

  state.counters["requests"] = static_cast<double>(m);
  state.counters["compute_thr"] = static_cast<double>(thread_count<Sch>(state));

  std::int64_t expect = m * expected_per_request();

  Sch compute_pool = make_scheduler<Sch>(state);

  lf_bench::bench(state, static_cast<std::int64_t>(thread_count<Sch>(state)), expect, [&]() -> std::int64_t {
    return lf::schedule(compute_pool, fan_out_baseline<Sch>{}, m).get();
  });
}

} // namespace

// prefix = requests  →  macro uses requests_test / requests_base
LIBFORK_BENCH_ALL_MT(run_with_io, request_io, requests, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run_baseline, request_baseline, requests, mono_busy_pool)

LIBFORK_BENCH_ALL_MT(run_with_io, request_io, requests, poly_busy_pool)
LIBFORK_BENCH_ALL_MT(run_baseline, request_baseline, requests, poly_busy_pool)
