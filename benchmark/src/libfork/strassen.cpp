#include <benchmark/benchmark.h>

#include "matmul.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr unsigned strassen_cutoff = 64;

void mat_add(float const *A, unsigned sa, float const *B, unsigned sb, float *Out, unsigned so, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      Out[i * so + j] = A[i * sa + j] + B[i * sb + j];
    }
  }
}

void mat_sub(float const *A, unsigned sa, float const *B, unsigned sb, float *Out, unsigned so, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      Out[i * so + j] = A[i * sa + j] - B[i * sb + j];
    }
  }
}

void naive_multiply(
    float const *A, unsigned sa, float const *B, unsigned sb, float *C, unsigned sc, unsigned n) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      float sum = 0;
      for (unsigned k = 0; k < n; ++k) {
        sum += A[i * sa + k] * B[k * sb + j];
      }
      C[i * sc + j] = sum;
    }
  }
}

struct strassen_task {
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
      naive_multiply(A, sa, B, sb, C, sc, n);
      co_return;
    }

    unsigned m = n / 2;

    auto block = [m](auto *p, unsigned s, unsigned i, unsigned j) {
      return p + i * m * s + j * m;
    };

    float const *A11 = block(A, sa, 0, 0);
    float const *A12 = block(A, sa, 0, 1);
    float const *A21 = block(A, sa, 1, 0);
    float const *A22 = block(A, sa, 1, 1);
    float const *B11 = block(B, sb, 0, 0);
    float const *B12 = block(B, sb, 0, 1);
    float const *B21 = block(B, sb, 1, 0);
    float const *B22 = block(B, sb, 1, 1);
    float *C11 = block(C, sc, 0, 0);
    float *C12 = block(C, sc, 0, 1);
    float *C21 = block(C, sc, 1, 0);
    float *C22 = block(C, sc, 1, 1);

    std::size_t block_size = static_cast<std::size_t>(m) * m;
    std::vector<float> buf(block_size * 17);
    float *S1 = buf.data();
    float *S2 = S1 + block_size;
    float *S3 = S2 + block_size;
    float *S4 = S3 + block_size;
    float *S5 = S4 + block_size;
    float *S6 = S5 + block_size;
    float *S7 = S6 + block_size;
    float *S8 = S7 + block_size;
    float *S9 = S8 + block_size;
    float *S10 = S9 + block_size;
    float *M1 = S10 + block_size;
    float *M2 = M1 + block_size;
    float *M3 = M2 + block_size;
    float *M4 = M3 + block_size;
    float *M5 = M4 + block_size;
    float *M6 = M5 + block_size;
    float *M7 = M6 + block_size;

    mat_add(A11, sa, A22, sa, S1, m, m);
    mat_add(B11, sb, B22, sb, S2, m, m);
    mat_add(A21, sa, A22, sa, S3, m, m);
    mat_sub(B12, sb, B22, sb, S4, m, m);
    mat_sub(B21, sb, B11, sb, S5, m, m);
    mat_add(A11, sa, A12, sa, S6, m, m);
    mat_sub(A21, sa, A11, sa, S7, m, m);
    mat_add(B11, sb, B12, sb, S8, m, m);
    mat_sub(A12, sa, A22, sa, S9, m, m);
    mat_add(B21, sb, B22, sb, S10, m, m);

    {
      auto scp = co_await lf::scope();
      co_await scp.fork(strassen_task{}, S1, m, S2, m, M1, m, m);
      co_await scp.fork(strassen_task{}, S3, m, B11, sb, M2, m, m);
      co_await scp.fork(strassen_task{}, A11, sa, S4, m, M3, m, m);
      co_await scp.fork(strassen_task{}, A22, sa, S5, m, M4, m, m);
      co_await scp.fork(strassen_task{}, S6, m, B22, sb, M5, m, m);
      co_await scp.fork(strassen_task{}, S7, m, S8, m, M6, m, m);
      co_await scp.call(strassen_task{}, S9, m, S10, m, M7, m, m);
      co_await scp.join();
    }

    for (unsigned i = 0; i < m; ++i) {
      for (unsigned j = 0; j < m; ++j) {
        std::size_t k = static_cast<std::size_t>(i) * m + j;
        C11[i * sc + j] = M1[k] + M4[k] - M5[k] + M7[k];
        C12[i * sc + j] = M3[k] + M5[k];
        C21[i * sc + j] = M2[k] + M4[k];
        C22[i * sc + j] = M1[k] - M2[k] + M3[k] + M6[k];
      }
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_matmul(state, 1e-3f, [&](float const *A, float const *B, float *C, unsigned n) {
    lf::schedule(scheduler, strassen_task{}, A, n, B, n, C, n, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, strassen, strassen, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, strassen, strassen, poly_busy_pool)
