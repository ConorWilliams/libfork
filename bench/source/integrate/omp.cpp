#include <thread>
#include <vector>

#include <benchmark/benchmark.h>

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

#pragma omp task untied default(shared)
  area_x1x0 = integrate(x1, y1, x0, y0, area_x1x0);
  area_x0x2 = integrate(x0, y0, x2, y2, area_x0x2);
#pragma omp taskwait

  return area_x1x0 + area_x0x2;
}

void integrate_omp(benchmark::State &state) {

  volatile double out;

  for (auto _ : state) {
#pragma omp parallel num_threads(state.range(0))
#pragma omp single
    out = integrate(0, fn(0), n, fn(n), 0);
  }

  double expect = integral_fn(0, n);

  if (out - expect < epsilon && expect - out < epsilon) {
    return;
  }

  std::cout << "error: " << out << "!=" << expect << std::endl;
}

} // namespace

BENCHMARK(integrate_omp)->DenseRange(1, num_threads())->UseRealTime();
