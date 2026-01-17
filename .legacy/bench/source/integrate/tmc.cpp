#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

#include "tmc/all_headers.hpp"

namespace {

using namespace tmc;

auto integrate(double x1, double y1, double x2, double y2, double area) -> task<double> {
  //
  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = fn(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < epsilon && area - area_x1x2 < epsilon) {
    co_return area_x1x2;
  }

  auto xt = spawn(integrate(x1, y1, x0, y0, area_x1x0)).run_early();
  auto y = co_await integrate(x0, y0, x2, y2, area_x0x2);
  auto x = co_await xt;

  co_return x + y;
};

void integrate_tmc(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["integrate_n"] = n;
  state.counters["integrate_epsilon"] = epsilon;

  volatile double out;

  tmc::cpu_executor().set_thread_count(state.range(0)).init();

  for (auto _ : state) {
    out = tmc::post_waitable(tmc::cpu_executor(), integrate(0, fn(0), n, fn(n), 0), 0).get();
  }

  tmc::cpu_executor().teardown();

  double expect = integral_fn(0, n);

  if (out - expect < epsilon && expect - out < epsilon) {
    return;
  }

  std::cout << "error: " << out << "!=" << expect << std::endl;
}

} // namespace

BENCHMARK(integrate_tmc)->Apply(targs)->UseRealTime();
