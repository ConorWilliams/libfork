#include <benchmark/benchmark.h>

#include "integrate.hpp"
#include "macros.hpp"

import std;

namespace {

struct simpson_estimate {
  double mid;
  double f_mid;
  double area;
};

auto integrate_simpson(double x1, double y1, double x2, double y2) -> simpson_estimate {
  double mid = (x1 + x2) / 2.0;
  double f_mid = integrate_fn(mid);
  double area = (x2 - x1) / 6.0 * (y1 + 4.0 * f_mid + y2);
  return {mid, f_mid, area};
}

auto integrate_recurse(double x1, double y1, double x2, double y2, double eps, simpson_estimate whole)
    -> double {

  auto left = integrate_simpson(x1, y1, whole.mid, whole.f_mid);
  auto right = integrate_simpson(whole.mid, whole.f_mid, x2, y2);
  double delta = left.area + right.area - whole.area;

  if (std::abs(delta) <= 15.0 * eps) {
    return left.area + right.area + delta / 15.0;
  }

  return integrate_recurse(x1, y1, whole.mid, whole.f_mid, eps / 2.0, left) +
         integrate_recurse(whole.mid, whole.f_mid, x2, y2, eps / 2.0, right);
}

template <typename = void>
void integrate_serial(benchmark::State &state) {
  run_integrate(state, [](double upper) {
    double lower = 0.0;
    double lower_y = integrate_fn(lower);
    double upper_y = integrate_fn(upper);
    auto whole = integrate_simpson(lower, lower_y, upper, upper_y);
    return integrate_recurse(lower, lower_y, upper, upper_y, integrate_epsilon, whole);
  });
}

} // namespace

BENCH_ALL(integrate_serial, serial, integrate, integrate)
