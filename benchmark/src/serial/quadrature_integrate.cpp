#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "quadrature_integrate.hpp"

import std;

namespace {

auto quadrature_integrate_serial(double x1, double y1, double x2, double y2, double area, int depth)
    -> quadrature_integrate_result {
  double half = (x2 - x1) / 2.0;
  double x0 = x1 + half;
  double y0 = quadrature_integrate_fn(x0);

  double lhs = quadrature_integrate_area(x1, y1, x0, y0);
  double rhs = quadrature_integrate_area(x0, y0, x2, y2);
  double split = lhs + rhs;

  if (std::abs(split - area) < quadrature_integrate_epsilon) {
    return {.area = split, .leaves = 1, .depth = depth};
  }

  auto lhs_result = quadrature_integrate_serial(x1, y1, x0, y0, lhs, depth + 1);
  auto rhs_result = quadrature_integrate_serial(x0, y0, x2, y2, rhs, depth + 1);

  return {
      .area = lhs_result.area + rhs_result.area,
      .leaves = lhs_result.leaves + rhs_result.leaves,
      .depth = std::max(lhs_result.depth, rhs_result.depth),
  };
}

template <typename = void>
void quadrature_integrate_serial_bench(benchmark::State &state) {
  run_quadrature_integrate(state, quadrature_integrate_serial);
}

} // namespace

BENCH_ALL(quadrature_integrate_serial_bench, serial, quadrature_integrate, quadrature)
