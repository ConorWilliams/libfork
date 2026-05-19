#include <benchmark/benchmark.h>

#include "winograd.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct winograd_prepare_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         float const *A11,
                         unsigned sa,
                         float const *A12,
                         float const *A21,
                         float const *A22,
                         float const *B11,
                         unsigned sb,
                         float const *B12,
                         float const *B21,
                         float const *B22,
                         winograd_blocks blocks,
                         unsigned m,
                         unsigned row_begin,
                         unsigned row_end) -> lf::task<void, Context> {
    if (row_end - row_begin <= winograd_loop_cutoff) {
      winograd_prepare_range(A11, sa, A12, A21, A22, B11, sb, B12, B21, B22, blocks, m, row_begin, row_end);
      co_return;
    }

    unsigned mid = row_begin + (row_end - row_begin) / 2;
    auto scp = co_await lf::scope();
    co_await scp.fork(
        winograd_prepare_task{}, A11, sa, A12, A21, A22, B11, sb, B12, B21, B22, blocks, m, row_begin, mid);
    co_await scp.call(
        winograd_prepare_task{}, A11, sa, A12, A21, A22, B11, sb, B12, B21, B22, blocks, m, mid, row_end);
    co_await scp.join();
  }
};

struct winograd_combine_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         float *C11,
                         float *C12,
                         float *C21,
                         float *C22,
                         unsigned sc,
                         winograd_blocks blocks,
                         unsigned m,
                         unsigned row_begin,
                         unsigned row_end) -> lf::task<void, Context> {
    if (row_end - row_begin <= winograd_loop_cutoff) {
      winograd_combine_range(C11, C12, C21, C22, sc, blocks, m, row_begin, row_end);
      co_return;
    }

    unsigned mid = row_begin + (row_end - row_begin) / 2;
    auto scp = co_await lf::scope();
    co_await scp.fork(winograd_combine_task{}, C11, C12, C21, C22, sc, blocks, m, row_begin, mid);
    co_await scp.call(winograd_combine_task{}, C11, C12, C21, C22, sc, blocks, m, mid, row_end);
    co_await scp.join();
  }
};

struct winograd_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         float const *A,
                         unsigned sa,
                         float const *B,
                         unsigned sb,
                         float *C,
                         unsigned sc,
                         unsigned n) -> lf::task<void, Context> {
    if (n <= winograd_naive_cutoff) {
      matrix_multiply_basecase<false>(A, sa, B, sb, C, sc, n);
      co_return;
    }

    unsigned m = n / 2;
    float const *A11 = strassen_block(A, sa, m, 0, 0);
    float const *A12 = strassen_block(A, sa, m, 0, 1);
    float const *A21 = strassen_block(A, sa, m, 1, 0);
    float const *A22 = strassen_block(A, sa, m, 1, 1);
    float const *B11 = strassen_block(B, sb, m, 0, 0);
    float const *B12 = strassen_block(B, sb, m, 0, 1);
    float const *B21 = strassen_block(B, sb, m, 1, 0);
    float const *B22 = strassen_block(B, sb, m, 1, 1);
    float *C11 = strassen_block(C, sc, m, 0, 0);
    float *C12 = strassen_block(C, sc, m, 0, 1);
    float *C21 = strassen_block(C, sc, m, 1, 0);
    float *C22 = strassen_block(C, sc, m, 1, 1);

    std::vector<float> buf(winograd_scratch_size(m));
    auto blocks = winograd_scratch_blocks(buf.data(), m);

    {
      auto scp = co_await lf::scope();
      co_await scp.call(
          winograd_prepare_task{}, A11, sa, A12, A21, A22, B11, sb, B12, B21, B22, blocks, m, 0U, m);
      co_await scp.join();
    }

    {
      auto scp = co_await lf::scope();
      co_await scp.fork(winograd_fn{}, A11, sa, B11, sb, blocks.M2, m, m);
      co_await scp.fork(winograd_fn{}, blocks.S1, m, blocks.S5, m, blocks.M5, m, m);
      co_await scp.fork(winograd_fn{}, blocks.S2, m, blocks.S6, m, blocks.T1, m, m);
      co_await scp.fork(winograd_fn{}, blocks.S3, m, blocks.S7, m, C22, sc, m);
      co_await scp.fork(winograd_fn{}, A12, sa, B21, sb, C11, sc, m);
      co_await scp.fork(winograd_fn{}, blocks.S4, m, B22, sb, C12, sc, m);
      co_await scp.call(winograd_fn{}, A22, sa, blocks.S8, m, C21, sc, m);
      co_await scp.join();
    }

    {
      auto scp = co_await lf::scope();
      co_await scp.call(winograd_combine_task{}, C11, C12, C21, C22, sc, blocks, m, 0U, m);
      co_await scp.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_matrix_multiply(state, threads, 1e-3f, [&](float const *A, float const *B, float *C, unsigned n) {
    lf::schedule(scheduler, winograd_fn{}, A, n, B, n, C, n, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, winograd, winograd, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, winograd, winograd, poly_busy_pool)
