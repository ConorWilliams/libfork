#include <benchmark/benchmark.h>

#include "transpose.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct transpose_block_swap_fn {
  template <lf::worker_context Context>
  static auto
  operator()(lf::env<Context>, float *A, float *B, unsigned n, unsigned s) -> lf::task<void, Context> {

    if (n <= transpose_cutoff) {
      transpose_block_swap_basecase(A, B, n, s);
      co_return;
    }

    unsigned const m = n / 2;
    unsigned const o00 = 0;
    unsigned const o01 = m;
    unsigned const o10 = m * s;
    unsigned const o11 = m * s + m;

    auto sc = co_await lf::scope();
    co_await sc.fork(transpose_block_swap_fn{}, A + o00, B + o00, m, s);
    co_await sc.fork(transpose_block_swap_fn{}, A + o01, B + o01, m, s);
    co_await sc.fork(transpose_block_swap_fn{}, A + o10, B + o10, m, s);
    co_await sc.call(transpose_block_swap_fn{}, A + o11, B + o11, m, s);
    co_await sc.join();
  }
};

struct transpose_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float *A, unsigned n, unsigned s) -> lf::task<void, Context> {

    if (n <= transpose_cutoff) {
      transpose_basecase(A, n, s);
      co_return;
    }

    unsigned const m = n / 2;
    unsigned const o00 = 0;
    unsigned const o01 = m;
    unsigned const o10 = m * s;
    unsigned const o11 = m * s + m;

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(transpose_fn{}, A + o00, m, s);
      co_await sc.fork(transpose_fn{}, A + o01, m, s);
      co_await sc.fork(transpose_fn{}, A + o10, m, s);
      co_await sc.call(transpose_fn{}, A + o11, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(transpose_block_swap_fn{}, A + o01, A + o10, m, s);
      co_await sc.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_transpose(state, threads, [&](float *A, unsigned n) {
    lf::schedule(scheduler, transpose_fn{}, A, n, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, transpose, transpose, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, transpose, transpose, poly_busy_pool)
