#include <benchmark/benchmark.h>

#include "cholesky.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct mul_and_subT_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         unsigned size,
                         bool lower,
                         cholesky_node const *a,
                         cholesky_node const *b,
                         cholesky_matrix *r) -> lf::task<void, Context> {
    if (a == nullptr || b == nullptr) {
      co_return;
    }

    if (*r == nullptr) {
      *r = std::make_unique<cholesky_node>();
    }

    if (size == cholesky_final_cutoff) {
      lower ? cholesky_block_schur_half(**r, *a, *b) : cholesky_block_schur_full(**r, *a, *b);
      co_return;
    }

    unsigned half = size / 2;
    auto sc = co_await lf::scope();

    co_await sc.fork(mul_and_subT_task{},
                     half,
                     lower,
                     a->child[cholesky_00].get(),
                     b->child[cholesky_transpose_quadrant[cholesky_00]].get(),
                     &(*r)->child[cholesky_00]);
    if (!lower) {
      co_await sc.fork(mul_and_subT_task{},
                       half,
                       false,
                       a->child[cholesky_00].get(),
                       b->child[cholesky_transpose_quadrant[cholesky_01]].get(),
                       &(*r)->child[cholesky_01]);
    }
    co_await sc.fork(mul_and_subT_task{},
                     half,
                     false,
                     a->child[cholesky_10].get(),
                     b->child[cholesky_transpose_quadrant[cholesky_00]].get(),
                     &(*r)->child[cholesky_10]);
    co_await sc.call(mul_and_subT_task{},
                     half,
                     lower,
                     a->child[cholesky_10].get(),
                     b->child[cholesky_transpose_quadrant[cholesky_01]].get(),
                     &(*r)->child[cholesky_11]);
    co_await sc.join();

    co_await sc.fork(mul_and_subT_task{},
                     half,
                     lower,
                     a->child[cholesky_01].get(),
                     b->child[cholesky_transpose_quadrant[cholesky_10]].get(),
                     &(*r)->child[cholesky_00]);
    if (!lower) {
      co_await sc.fork(mul_and_subT_task{},
                       half,
                       false,
                       a->child[cholesky_01].get(),
                       b->child[cholesky_transpose_quadrant[cholesky_11]].get(),
                       &(*r)->child[cholesky_01]);
    }
    co_await sc.fork(mul_and_subT_task{},
                     half,
                     false,
                     a->child[cholesky_11].get(),
                     b->child[cholesky_transpose_quadrant[cholesky_10]].get(),
                     &(*r)->child[cholesky_10]);
    co_await sc.call(mul_and_subT_task{},
                     half,
                     lower,
                     a->child[cholesky_11].get(),
                     b->child[cholesky_transpose_quadrant[cholesky_11]].get(),
                     &(*r)->child[cholesky_11]);
    co_await sc.join();
  }
};

struct backsub_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, unsigned size, cholesky_matrix *a, cholesky_node const *l)
      -> lf::task<void, Context> {
    if (*a == nullptr || l == nullptr) {
      co_return;
    }

    if (size == cholesky_final_cutoff) {
      cholesky_block_backsub(**a, *l);
      co_return;
    }

    unsigned half = size / 2;
    auto sc = co_await lf::scope();

    co_await sc.fork(backsub_task{}, half, &(*a)->child[cholesky_00], l->child[cholesky_00].get());
    co_await sc.call(backsub_task{}, half, &(*a)->child[cholesky_10], l->child[cholesky_00].get());
    co_await sc.join();

    co_await sc.fork(mul_and_subT_task{},
                     half,
                     false,
                     (*a)->child[cholesky_00].get(),
                     l->child[cholesky_10].get(),
                     &(*a)->child[cholesky_01]);
    co_await sc.call(mul_and_subT_task{},
                     half,
                     false,
                     (*a)->child[cholesky_10].get(),
                     l->child[cholesky_10].get(),
                     &(*a)->child[cholesky_11]);
    co_await sc.join();

    co_await sc.fork(backsub_task{}, half, &(*a)->child[cholesky_01], l->child[cholesky_11].get());
    co_await sc.call(backsub_task{}, half, &(*a)->child[cholesky_11], l->child[cholesky_11].get());
    co_await sc.join();
  }
};

struct cholesky_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, unsigned size, cholesky_matrix *a) -> lf::task<void, Context> {
    if (*a == nullptr) {
      co_return;
    }

    if (size == cholesky_final_cutoff) {
      cholesky_block_factor(**a);
      co_return;
    }

    unsigned half = size / 2;
    if ((*a)->child[cholesky_10] == nullptr) {
      auto sc = co_await lf::scope();
      co_await sc.fork(cholesky_task{}, half, &(*a)->child[cholesky_00]);
      co_await sc.call(cholesky_task{}, half, &(*a)->child[cholesky_11]);
      co_await sc.join();
    } else {
      {
        auto sc = co_await lf::scope();
        co_await sc.call(cholesky_task{}, half, &(*a)->child[cholesky_00]);
        co_await sc.join();
      }
      {
        auto sc = co_await lf::scope();
        co_await sc.call(backsub_task{}, half, &(*a)->child[cholesky_10], (*a)->child[cholesky_00].get());
        co_await sc.join();
      }
      {
        auto sc = co_await lf::scope();
        co_await sc.call(mul_and_subT_task{},
                         half,
                         true,
                         (*a)->child[cholesky_10].get(),
                         (*a)->child[cholesky_10].get(),
                         &(*a)->child[cholesky_11]);
        co_await sc.join();
      }
      {
        auto sc = co_await lf::scope();
        co_await sc.call(cholesky_task{}, half, &(*a)->child[cholesky_11]);
        co_await sc.join();
      }
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_cholesky(state, threads, [&](cholesky_matrix &matrix, unsigned size) {
    lf::schedule(scheduler, cholesky_task{}, size, &matrix).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, cholesky, cholesky, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, cholesky, cholesky, poly_busy_pool)
