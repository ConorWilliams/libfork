#include <benchmark/benchmark.h>

#include "cholesky.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr unsigned cholesky_row_cutoff = 16;

struct solve_rows_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         double *A10,
                         double const *L00,
                         unsigned row_begin,
                         unsigned row_end,
                         unsigned m,
                         unsigned s) -> lf::task<void, Context> {
    if (row_end - row_begin <= cholesky_row_cutoff) {
      cholesky_solve_rows(A10, L00, row_begin, row_end, m, s);
      co_return;
    }

    unsigned mid = row_begin + (row_end - row_begin) / 2;
    auto sc = co_await lf::scope();
    co_await sc.fork(solve_rows_task{}, A10, L00, row_begin, mid, m, s);
    co_await sc.call(solve_rows_task{}, A10, L00, mid, row_end, m, s);
    co_await sc.join();
  }
};

struct schur_rows_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         double *A11,
                         double const *A10,
                         unsigned row_begin,
                         unsigned row_end,
                         unsigned m,
                         unsigned s) -> lf::task<void, Context> {
    if (row_end - row_begin <= cholesky_row_cutoff) {
      cholesky_schur_rows(A11, A10, row_begin, row_end, m, s);
      co_return;
    }

    unsigned mid = row_begin + (row_end - row_begin) / 2;
    auto sc = co_await lf::scope();
    co_await sc.fork(schur_rows_task{}, A11, A10, row_begin, mid, m, s);
    co_await sc.call(schur_rows_task{}, A11, A10, mid, row_end, m, s);
    co_await sc.join();
  }
};

struct cholesky_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, double *A, unsigned n, unsigned s) -> lf::task<void, Context> {
    if (n <= cholesky_cutoff) {
      cholesky_basecase(A, n, s);
      co_return;
    }

    unsigned m = n / 2;
    double *A00 = A;
    double *A10 = A + static_cast<std::size_t>(m) * s;
    double *A11 = A + static_cast<std::size_t>(m) * s + m;

    {
      auto sc = co_await lf::scope();
      co_await sc.call(cholesky_task{}, A00, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(solve_rows_task{}, A10, A00, 0U, m, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(schur_rows_task{}, A11, A10, 0U, m, m, s);
      co_await sc.join();
    }

    {
      auto sc = co_await lf::scope();
      co_await sc.call(cholesky_task{}, A11, m, s);
      co_await sc.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_cholesky(state, threads, [&](double *A, unsigned n, unsigned s) {
    lf::schedule(scheduler, cholesky_task{}, A, n, s).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, cholesky, cholesky, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, cholesky, cholesky, poly_busy_pool)
