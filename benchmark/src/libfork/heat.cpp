#include <benchmark/benchmark.h>

#include "heat.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr std::size_t heat_row_cutoff = 1;

struct heat_diffuse_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         heat_problem const *problem,
                         double const *in,
                         double *out,
                         std::size_t row_begin,
                         std::size_t row_end,
                         double t) -> lf::task<void, Context> {
    if (row_end - row_begin <= heat_row_cutoff) {
      heat_diffuse_rows(*problem, in, out, row_begin, row_end, t);
      co_return;
    }

    std::size_t mid = row_begin + (row_end - row_begin) / 2;
    auto sc = co_await lf::scope();
    co_await sc.fork(heat_diffuse_task{}, problem, in, out, row_begin, mid, t);
    co_await sc.call(heat_diffuse_task{}, problem, in, out, mid, row_end, t);
    co_await sc.join();
  }
};

struct heat_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, heat_problem *problem) -> lf::task<void, Context> {
    double t = heat_t_lower;

    for (std::size_t i = 1; i <= heat_iters; i += 2) {
      t += problem->dt;
      {
        auto sc = co_await lf::scope();
        co_await sc.call(heat_diffuse_task{}, problem, problem->even.data(), problem->odd.data(), 0, problem->nx, t);
        co_await sc.join();
      }
      t += problem->dt;
      {
        auto sc = co_await lf::scope();
        co_await sc.call(heat_diffuse_task{}, problem, problem->odd.data(), problem->even.data(), 0, problem->nx, t);
        co_await sc.join();
      }
    }

    if (heat_iters % 2 != 0) {
      t += problem->dt;
      auto sc = co_await lf::scope();
      co_await sc.call(heat_diffuse_task{}, problem, problem->even.data(), problem->odd.data(), 0, problem->nx, t);
      co_await sc.join();
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_heat(state, threads, [&](heat_problem &problem) {
    lf::schedule(scheduler, heat_task{}, &problem).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, heat, heat, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, heat, heat, poly_busy_pool)
