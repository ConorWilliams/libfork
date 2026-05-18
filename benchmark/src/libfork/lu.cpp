#include <benchmark/benchmark.h>

#include "lu.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct schur_task {
  template <lf::worker_context Context>
  static auto
  operator()(lf::env<Context>, lu_block *M, lu_block *V, lu_block *W, unsigned stride, unsigned nb)
      -> lf::task<void, Context> {
    if (nb == 1) {
      lu_block_schur(*M, *V, *W);
      co_return;
    }

    unsigned h = nb / 2;
    auto *M00 = lu_block_at(M, stride, 0, 0);
    auto *M01 = lu_block_at(M, stride, 0, h);
    auto *M10 = lu_block_at(M, stride, h, 0);
    auto *M11 = lu_block_at(M, stride, h, h);
    auto *V00 = lu_block_at(V, stride, 0, 0);
    auto *V01 = lu_block_at(V, stride, 0, h);
    auto *V10 = lu_block_at(V, stride, h, 0);
    auto *V11 = lu_block_at(V, stride, h, h);
    auto *W00 = lu_block_at(W, stride, 0, 0);
    auto *W01 = lu_block_at(W, stride, 0, h);
    auto *W10 = lu_block_at(W, stride, h, 0);
    auto *W11 = lu_block_at(W, stride, h, h);

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(schur_task{}, M00, V00, W00, stride, h);
      co_await sc.fork(schur_task{}, M01, V00, W01, stride, h);
      co_await sc.fork(schur_task{}, M10, V10, W00, stride, h);
      co_await sc.call(schur_task{}, M11, V10, W01, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(schur_task{}, M00, V01, W10, stride, h);
      co_await sc.fork(schur_task{}, M01, V01, W11, stride, h);
      co_await sc.fork(schur_task{}, M10, V11, W10, stride, h);
      co_await sc.call(schur_task{}, M11, V11, W11, stride, h);
      co_await sc.join();
    }
  }
};

struct lower_solve_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, lu_block *M, lu_block *L, unsigned stride, unsigned nb)
      -> lf::task<void, Context> {
    if (nb == 1) {
      lu_block_lower_solve(*M, *L);
      co_return;
    }

    unsigned h = nb / 2;
    auto *M00 = lu_block_at(M, stride, 0, 0);
    auto *M01 = lu_block_at(M, stride, 0, h);
    auto *M10 = lu_block_at(M, stride, h, 0);
    auto *M11 = lu_block_at(M, stride, h, h);
    auto *L00 = lu_block_at(L, stride, 0, 0);
    auto *L10 = lu_block_at(L, stride, h, 0);
    auto *L11 = lu_block_at(L, stride, h, h);

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(lower_solve_task{}, M00, L00, stride, h);
      co_await sc.call(lower_solve_task{}, M01, L00, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(schur_task{}, M10, L10, M00, stride, h);
      co_await sc.call(schur_task{}, M11, L10, M01, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(lower_solve_task{}, M10, L11, stride, h);
      co_await sc.call(lower_solve_task{}, M11, L11, stride, h);
      co_await sc.join();
    }
  }
};

struct upper_solve_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, lu_block *M, lu_block *U, unsigned stride, unsigned nb)
      -> lf::task<void, Context> {
    if (nb == 1) {
      lu_block_upper_solve(*M, *U);
      co_return;
    }

    unsigned h = nb / 2;
    auto *M00 = lu_block_at(M, stride, 0, 0);
    auto *M01 = lu_block_at(M, stride, 0, h);
    auto *M10 = lu_block_at(M, stride, h, 0);
    auto *M11 = lu_block_at(M, stride, h, h);
    auto *U00 = lu_block_at(U, stride, 0, 0);
    auto *U01 = lu_block_at(U, stride, 0, h);
    auto *U11 = lu_block_at(U, stride, h, h);

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(upper_solve_task{}, M00, U00, stride, h);
      co_await sc.call(upper_solve_task{}, M10, U00, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(schur_task{}, M01, M00, U01, stride, h);
      co_await sc.call(schur_task{}, M11, M10, U01, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(upper_solve_task{}, M01, U11, stride, h);
      co_await sc.call(upper_solve_task{}, M11, U11, stride, h);
      co_await sc.join();
    }
  }
};

struct lu_task {
  template <lf::worker_context Context>
  static auto
  operator()(lf::env<Context>, lu_block *M, unsigned stride, unsigned nb) -> lf::task<void, Context> {
    if (nb == 1) {
      lu_block_lu(*M);
      co_return;
    }

    unsigned h = nb / 2;
    auto *M00 = lu_block_at(M, stride, 0, 0);
    auto *M01 = lu_block_at(M, stride, 0, h);
    auto *M10 = lu_block_at(M, stride, h, 0);
    auto *M11 = lu_block_at(M, stride, h, h);

    {
      auto sc = co_await lf::scope();
      co_await sc.call(lu_task{}, M00, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.fork(lower_solve_task{}, M01, M00, stride, h);
      co_await sc.call(upper_solve_task{}, M10, M00, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(schur_task{}, M11, M10, M01, stride, h);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(lu_task{}, M11, stride, h);
      co_await sc.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_lu(state, threads, [&](lu_block *M, unsigned blocks) {
    lf::schedule(scheduler, lu_task{}, M, blocks, blocks).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, lu, lu, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, lu, lu, poly_busy_pool)
