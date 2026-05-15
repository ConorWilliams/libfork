#include <benchmark/benchmark.h>

#include "integrate.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct integrate_task {
  template <lf::worker_context Context>
  static auto
  operator()(lf::env<Context>, double x1, double y1, double x2, double y2, double area)
      -> lf::task<double, Context> {

    double half = (x2 - x1) / 2;
    double x0 = x1 + half;
    double y0 = integrate_fn(x0);

    double area_x1x0 = (y1 + y0) / 2 * half;
    double area_x0x2 = (y0 + y2) / 2 * half;
    double area_x1x2 = area_x1x0 + area_x0x2;

    if (std::abs(area_x1x2 - area) < integrate_epsilon) {
      co_return area_x1x2;
    }

    double lhs = 0;
    double rhs = 0;

    auto sc = co_await lf::scope();
    co_await sc.fork(&lhs, integrate_task{}, x1, y1, x0, y0, area_x1x0);
    co_await sc.call(&rhs, integrate_task{}, x0, y0, x2, y2, area_x0x2);
    co_await sc.join();

    co_return lhs + rhs;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_integrate(state, [&](double upper) {
    return lf::schedule(scheduler, integrate_task{}, 0.0, integrate_fn(0), upper, integrate_fn(upper), 0.0).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, integrate, integrate, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, integrate, integrate, poly_busy_pool)
