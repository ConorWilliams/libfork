#include <benchmark/benchmark.h>

#include "strassen.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

template <bool Add>
struct strassen_mat_sum_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         float const *A,
                         unsigned sa,
                         float const *B,
                         unsigned sb,
                         float *Out,
                         unsigned so,
                         unsigned m) -> lf::task<void, Context> {
    if constexpr (Add) {
      strassen_mat_add(A, sa, B, sb, Out, so, m);
    } else {
      strassen_mat_sub(A, sa, B, sb, Out, so, m);
    }
    co_return;
  }
};

struct strassen_combine_00_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float *C, unsigned sc, strassen_blocks blocks, unsigned m)
      -> lf::task<void, Context> {
    strassen_combine_00(C, sc, blocks, m);
    co_return;
  }
};

struct strassen_combine_01_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float *C, unsigned sc, strassen_blocks blocks, unsigned m)
      -> lf::task<void, Context> {
    strassen_combine_01(C, sc, blocks, m);
    co_return;
  }
};

struct strassen_combine_10_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float *C, unsigned sc, strassen_blocks blocks, unsigned m)
      -> lf::task<void, Context> {
    strassen_combine_10(C, sc, blocks, m);
    co_return;
  }
};

struct strassen_combine_11_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, float *C, unsigned sc, strassen_blocks blocks, unsigned m)
      -> lf::task<void, Context> {
    strassen_combine_11(C, sc, blocks, m);
    co_return;
  }
};

struct strassen_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         float const *A,
                         unsigned sa,
                         float const *B,
                         unsigned sb,
                         float *C,
                         unsigned sc,
                         unsigned n) -> lf::task<void, Context> {

    if (n <= strassen_cutoff) {
      matmul_basecase_multiply<false>(A, sa, B, sb, C, sc, n);
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

    std::vector<float> buf(strassen_scratch_size(m));
    auto blocks = strassen_scratch_blocks(buf.data(), m);

    {
      auto scp = co_await lf::scope();
      co_await scp.fork(strassen_mat_sum_fn<true>{}, A11, sa, A22, sa, blocks.S1, m, m);
      co_await scp.fork(strassen_mat_sum_fn<true>{}, B11, sb, B22, sb, blocks.S2, m, m);
      co_await scp.fork(strassen_mat_sum_fn<true>{}, A21, sa, A22, sa, blocks.S3, m, m);
      co_await scp.fork(strassen_mat_sum_fn<false>{}, B12, sb, B22, sb, blocks.S4, m, m);
      co_await scp.fork(strassen_mat_sum_fn<false>{}, B21, sb, B11, sb, blocks.S5, m, m);
      co_await scp.fork(strassen_mat_sum_fn<true>{}, A11, sa, A12, sa, blocks.S6, m, m);
      co_await scp.fork(strassen_mat_sum_fn<false>{}, A21, sa, A11, sa, blocks.S7, m, m);
      co_await scp.fork(strassen_mat_sum_fn<true>{}, B11, sb, B12, sb, blocks.S8, m, m);
      co_await scp.fork(strassen_mat_sum_fn<false>{}, A12, sa, A22, sa, blocks.S9, m, m);
      co_await scp.call(strassen_mat_sum_fn<true>{}, B21, sb, B22, sb, blocks.S10, m, m);
      co_await scp.join();
    }

    {
      auto scp = co_await lf::scope();
      co_await scp.fork(strassen_fn{}, blocks.S1, m, blocks.S2, m, blocks.M1, m, m);
      co_await scp.fork(strassen_fn{}, blocks.S3, m, B11, sb, blocks.M2, m, m);
      co_await scp.fork(strassen_fn{}, A11, sa, blocks.S4, m, blocks.M3, m, m);
      co_await scp.fork(strassen_fn{}, A22, sa, blocks.S5, m, blocks.M4, m, m);
      co_await scp.fork(strassen_fn{}, blocks.S6, m, B22, sb, blocks.M5, m, m);
      co_await scp.fork(strassen_fn{}, blocks.S7, m, blocks.S8, m, blocks.M6, m, m);
      co_await scp.call(strassen_fn{}, blocks.S9, m, blocks.S10, m, blocks.M7, m, m);
      co_await scp.join();
    }

    {
      auto scp = co_await lf::scope();
      co_await scp.fork(strassen_combine_00_fn{}, C11, sc, blocks, m);
      co_await scp.fork(strassen_combine_01_fn{}, C12, sc, blocks, m);
      co_await scp.fork(strassen_combine_10_fn{}, C21, sc, blocks, m);
      co_await scp.call(strassen_combine_11_fn{}, C22, sc, blocks, m);
      co_await scp.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_matmul(state, threads, 1e-3f, [&](float const *A, float const *B, float *C, unsigned n) {
    lf::schedule(scheduler, strassen_fn{}, A, n, B, n, C, n, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, strassen, strassen, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, strassen, strassen, poly_busy_pool)
