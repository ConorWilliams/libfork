#include <benchmark/benchmark.h>

#include "integrate.hpp"
#include "macros.hpp"

import std;

namespace {

auto integrate_recurse(double x1, double y1, double x2, double y2, double area) -> double {

  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = integrate_fn(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < integrate_epsilon && area - area_x1x2 < integrate_epsilon) {
    return area_x1x2;
  }

  area_x1x0 = integrate_recurse(x1, y1, x0, y0, area_x1x0);
  area_x0x2 = integrate_recurse(x0, y0, x2, y2, area_x0x2);

  return area_x1x0 + area_x0x2;
}

template <typename = void>
void integrate_serial(benchmark::State &state) {
  run_integrate(state, [](double upper) {
    return integrate_recurse(0, integrate_fn(0), upper, integrate_fn(upper), 0);
  });
}

} // namespace

BENCH_ALL(integrate_serial, serial, integrate, integrate)
