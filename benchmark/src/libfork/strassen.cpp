#include <benchmark/benchmark.h>

#include "strassen.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

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

    strassen_prepare(A11, A12, A21, A22, sa, B11, B12, B21, B22, sb, blocks, m);

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

    strassen_combine(C11, C12, C21, C22, sc, blocks, m);
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
