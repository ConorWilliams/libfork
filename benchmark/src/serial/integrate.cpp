#include <benchmark/benchmark.h>

#include "integrate.hpp"
#include "macros.hpp"

import std;

namespace {

auto integrate_recurse(
    double x1, double y1, double x2, double y2, double eps, simpson_estimate whole, int depth)
    -> integrate_result {

  auto lhs = integrate_simpson(x1, y1, whole.mid, whole.f_mid);
  auto rhs = integrate_simpson(whole.mid, whole.f_mid, x2, y2);

  double delta = lhs.area + rhs.area - whole.area;

  if (std::abs(delta) <= 15.0 * eps) {
    return {.area = lhs.area + rhs.area + delta / 15.0, .leaves = 1, .depth = depth};
  }

  auto lhs_result = integrate_recurse(x1, y1, whole.mid, whole.f_mid, eps / 2.0, lhs, depth + 1);
  auto rhs_result = integrate_recurse(whole.mid, whole.f_mid, x2, y2, eps / 2.0, rhs, depth + 1);

  return {
      .area = lhs_result.area + rhs_result.area,
      .leaves = lhs_result.leaves + rhs_result.leaves,
      .depth = std::max(lhs_result.depth, rhs_result.depth),
  };
}

template <typename = void>
void integrate_serial(benchmark::State &state) {
  run_integrate(state, [](double tolerance) {
    double lower_y = integrate_fn(integrate_lower);
    double upper_y = integrate_fn(integrate_upper);
    auto whole = integrate_simpson(integrate_lower, lower_y, integrate_upper, upper_y);
    return integrate_recurse(integrate_lower, lower_y, integrate_upper, upper_y, tolerance, whole, 0);
  });
}

} // namespace

BENCH_ALL(integrate_serial, serial, integrate, integrate)
