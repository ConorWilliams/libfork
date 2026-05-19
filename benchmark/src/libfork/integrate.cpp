#include <benchmark/benchmark.h>

#include "integrate.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct integrate_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         double x1,
                         double y1,
                         double x2,
                         double y2,
                         double eps,
                         simpson_estimate whole,
                         int depth) -> lf::task<integrate_result, Context> {

    auto lhs = integrate_simpson(x1, y1, whole.mid, whole.f_mid);
    auto rhs = integrate_simpson(whole.mid, whole.f_mid, x2, y2);

    double delta = lhs.area + rhs.area - whole.area;

    if (std::abs(delta) <= 15.0 * eps) {
      co_return {.area = lhs.area + rhs.area + delta / 15.0, .leaves = 1, .depth = depth};
    }

    integrate_result lhs_r{};
    integrate_result rhs_r{};

    auto sc = co_await lf::scope();
    co_await sc.fork(&lhs_r, integrate_task{}, x1, y1, whole.mid, whole.f_mid, eps / 2.0, lhs, depth + 1);
    co_await sc.call(&rhs_r, integrate_task{}, whole.mid, whole.f_mid, x2, y2, eps / 2.0, rhs, depth + 1);
    co_await sc.join();

    co_return {
        .area = lhs_r.area + rhs_r.area,
        .leaves = lhs_r.leaves + rhs_r.leaves,
        .depth = std::max(lhs_r.depth, rhs_r.depth),
    };
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_integrate(state, threads, [&](double tolerance) -> integrate_result {
    double lower_y = integrate_fn(integrate_lower);
    double upper_y = integrate_fn(integrate_upper);
    auto whole = integrate_simpson(integrate_lower, lower_y, integrate_upper, upper_y);
    return lf::schedule(scheduler,
                        integrate_task{},
                        integrate_lower,
                        lower_y,
                        integrate_upper,
                        upper_y,
                        tolerance,
                        whole,
                        0)
        .get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, integrate, integrate, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, integrate, integrate, poly_busy_pool)
