#include <benchmark/benchmark.h>

#include "quadrature_integrate.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct quadrature_integrate_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, double x1, double y1, double x2, double y2, double area, int depth)
      -> lf::task<quadrature_integrate_result, Context> {
    double half = (x2 - x1) / 2.0;
    double x0 = x1 + half;
    double y0 = quadrature_integrate_fn(x0);

    double lhs = quadrature_integrate_area(x1, y1, x0, y0);
    double rhs = quadrature_integrate_area(x0, y0, x2, y2);
    double split = lhs + rhs;

    if (std::abs(split - area) < quadrature_integrate_epsilon) {
      co_return {.area = split, .leaves = 1, .depth = depth};
    }

    quadrature_integrate_result lhs_result{};
    quadrature_integrate_result rhs_result{};

    auto sc = co_await lf::scope();
    co_await sc.fork(&lhs_result, quadrature_integrate_task{}, x1, y1, x0, y0, lhs, depth + 1);
    co_await sc.call(&rhs_result, quadrature_integrate_task{}, x0, y0, x2, y2, rhs, depth + 1);
    co_await sc.join();

    co_return {
        .area = lhs_result.area + rhs_result.area,
        .leaves = lhs_result.leaves + rhs_result.leaves,
        .depth = std::max(lhs_result.depth, rhs_result.depth),
    };
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_quadrature_integrate(
      state, threads, [&](double x1, double y1, double x2, double y2, double area, int depth) {
        return lf::schedule(scheduler, quadrature_integrate_task{}, x1, y1, x2, y2, area, depth).get();
      });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, quadrature_integrate, quadrature, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, quadrature_integrate, quadrature, poly_busy_pool)
