#include <benchmark/benchmark.h>

#include "matmul.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

enum matmul_quadrant : unsigned {
  matmul_q00,
  matmul_q10,
  matmul_q01,
  matmul_q11,
};

struct matmul_product_task;

template <matmul_quadrant Quadrant>
struct matmul_quadrant_task;

struct matmul_product_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float const *A, float const *B, float *R, unsigned n, unsigned s)
      -> lf::task<void, Context>;
};

template <matmul_quadrant Quadrant>
struct matmul_quadrant_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float const *A, float const *B, float *R, unsigned n, unsigned s)
      -> lf::task<void, Context>;
};

template <lf::worker_context Context>
auto matmul_product_task::operator()(
    lf::env<Context>, float const *A, float const *B, float *R, unsigned n, unsigned s)
    -> lf::task<void, Context> {
  if (n <= matmul_cutoff) {
    matmul_basecase_multiply<true>(A, B, R, n, s);
    co_return;
  }

  auto sc = co_await lf::scope();
  co_await sc.fork(matmul_quadrant_task<matmul_q00>{}, A, B, R, n, s);
  co_await sc.fork(matmul_quadrant_task<matmul_q10>{}, A, B, R, n, s);
  co_await sc.fork(matmul_quadrant_task<matmul_q01>{}, A, B, R, n, s);
  co_await sc.call(matmul_quadrant_task<matmul_q11>{}, A, B, R, n, s);
  co_await sc.join();
}

template <matmul_quadrant Quadrant>
template <lf::worker_context Context>
auto matmul_quadrant_task<Quadrant>::operator()(
    lf::env<Context>, float const *A, float const *B, float *R, unsigned n, unsigned s)
    -> lf::task<void, Context> {
  unsigned m = n / 2;

  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  if constexpr (Quadrant == matmul_q00) {
    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o00, B + o00, R + o00, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o01, B + o10, R + o00, m, s);
      co_await sc.join();
    }
  } else if constexpr (Quadrant == matmul_q10) {
    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o10, B + o00, R + o10, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o11, B + o10, R + o10, m, s);
      co_await sc.join();
    }
  } else if constexpr (Quadrant == matmul_q01) {
    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o00, B + o01, R + o01, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o01, B + o11, R + o01, m, s);
      co_await sc.join();
    }
  } else {
    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o10, B + o01, R + o11, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(matmul_product_task{}, A + o11, B + o11, R + o11, m, s);
      co_await sc.join();
    }
  }
}

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_matmul(state, threads, 1e-5f, [&](float const *A, float const *B, float *C, unsigned n) {
    lf::schedule(scheduler, matmul_product_task{}, A, B, C, n, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, matmul, matmul, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, matmul, matmul, poly_busy_pool)
