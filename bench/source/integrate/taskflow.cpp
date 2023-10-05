#include <benchmark/benchmark.h>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto integrate(double x1, double y1, double x2, double y2, double area, tf::Subflow &sbf) -> double {

  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = fn(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < epsilon && area - area_x1x2 < epsilon) {
    return area_x1x2;
  }

  sbf.emplace([&](tf::Subflow &sbf) {
    area_x1x0 = integrate(x1, y1, x0, y0, area_x1x0, sbf);
  });

  sbf.emplace([&](tf::Subflow &sbf) {
    area_x0x2 = integrate(x0, y0, x2, y2, area_x0x2, sbf);
  });

  sbf.join();

  return area_x1x0 + area_x0x2;
}

void integrate_taskflow(benchmark::State &state) {

  volatile double out;

  tf::Executor executor(state.range(0));

  tf::Taskflow taskflow;

  taskflow.emplace([&out](tf::Subflow &sbf) {
    out = integrate(0, fn(0), n, fn(n), 0, sbf);
  });

  for (auto _ : state) {
    executor.run(taskflow).wait();
  }

  double expect = integral_fn(0, n);

  if (out - expect < epsilon && expect - out < epsilon) {
    return;
  }

  std::cout << "error: " << out << "!=" << expect << std::endl;
}

} // namespace

BENCHMARK(integrate_taskflow)->DenseRange(1, num_threads())->UseRealTime();
