#include <benchmark/benchmark.h>

#include <tbb/global_control.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto integrate(double x1, double y1, double x2, double y2, double area) -> double {

  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = fn(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < epsilon && area - area_x1x2 < epsilon) {
    return area_x1x2;
  }

  tbb::task_group g;

  g.run([&] {
    area_x1x0 = integrate(x1, y1, x0, y0, area_x1x0);
  });

  area_x0x2 = integrate(x0, y0, x2, y2, area_x0x2);

  g.wait();

  return area_x1x0 + area_x0x2;
}

void integrate_tbb(benchmark::State &state) {

  // TBB uses (2MB) stacks by default
  tbb::global_control global_limit(tbb::global_control::thread_stack_size, 8 * 1024 * 1024);

  state.counters["green_threads"] = state.range(0);
  state.counters["integrate_n"] = n;
  state.counters["integrate_epsilon"] = epsilon;

  tbb::task_arena arena(state.range(0));

  volatile double out;

  for (auto _ : state) {
    out = arena.execute([&] {
      return integrate(0, fn(0), n, fn(n), 0);
    });
  }

  double expect = integral_fn(0, n);

  if (out - expect < epsilon && expect - out < epsilon) {
    return;
  }

  std::cout << "error: " << out << "!=" << expect << std::endl;
}

} // namespace

BENCHMARK(integrate_tbb)->Apply(targs)->UseRealTime();
