#include <benchmark/benchmark.h>

#include "rectmul.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct add_matrix_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, rectmul_block const *T, long ot, rectmul_block *R, long oR, long x, long y)
      -> lf::task<void, Context> {
    if (x + y == 2) {
      for (long i = 0; i < rectmul_block_size; ++i) {
        (*R)[static_cast<std::size_t>(i)] += (*T)[static_cast<std::size_t>(i)];
      }
      co_return;
    }

    auto sc = co_await lf::scope();
    if (x > y) {
      co_await sc.fork(add_matrix_task{}, T, ot, R, oR, x / 2, y);
      co_await sc.call(add_matrix_task{}, T + (x / 2) * ot, ot, R + (x / 2) * oR, oR, (x + 1) / 2, y);
    } else {
      co_await sc.fork(add_matrix_task{}, T, ot, R, oR, x, y / 2);
      co_await sc.call(add_matrix_task{}, T + (y / 2), ot, R + (y / 2), oR, x, (y + 1) / 2);
    }
    co_await sc.join();
  }
};

struct multiply_matrix_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         rectmul_block *A,
                         long oa,
                         rectmul_block *B,
                         long ob,
                         long x,
                         long y,
                         long z,
                         rectmul_block *R,
                         long oR,
                         int add) -> lf::task<void, Context> {
    if (x + y + z == 3) {
      if (add != 0) {
        rectmul_mult_add_block(*A, *B, *R);
      } else {
        rectmul_multiply_block(*A, *B, *R);
      }
      co_return;
    }

    if (x >= y && x >= z) {
      auto sc = co_await lf::scope();
      co_await sc.fork(multiply_matrix_task{}, A, oa, B, ob, x / 2, y, z, R, oR, add);
      co_await sc.call(
          multiply_matrix_task{}, A + (x / 2) * oa, oa, B, ob, (x + 1) / 2, y, z, R + (x / 2) * oR, oR, add);
      co_await sc.join();
    } else if (y > x && y > z) {
      std::vector<rectmul_block> tmp(static_cast<std::size_t>(x * z));

      {
        auto sc = co_await lf::scope();
        co_await sc.fork(multiply_matrix_task{}, A + (y / 2), oa, B + (y / 2) * ob, ob, x, (y + 1) / 2, z, R, oR, add);
        co_await sc.call(multiply_matrix_task{}, A, oa, B, ob, x, y / 2, z, tmp.data(), z, 0);
        co_await sc.join();
      }

      auto sc = co_await lf::scope();
      co_await sc.call(add_matrix_task{}, tmp.data(), z, R, oR, x, z);
      co_await sc.join();
    } else {
      auto sc = co_await lf::scope();
      co_await sc.fork(multiply_matrix_task{}, A, oa, B, ob, x, y, z / 2, R, oR, add);
      co_await sc.call(multiply_matrix_task{}, A, oa, B + (z / 2), ob, x, y, (z + 1) / 2, R + (z / 2), oR, add);
      co_await sc.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_rectmul(state, threads, [&](rectmul_block *A,
                                  long oa,
                                  rectmul_block *B,
                                  long ob,
                                  long x,
                                  long y,
                                  long z,
                                  rectmul_block *R,
                                  long oR,
                                  int add) {
    lf::schedule(scheduler, multiply_matrix_task{}, A, oa, B, ob, x, y, z, R, oR, add).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, rectmul, rectmul, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, rectmul, rectmul, poly_busy_pool)
