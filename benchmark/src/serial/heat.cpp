#include <benchmark/benchmark.h>

#include "heat.hpp"
#include "macros.hpp"

import std;

namespace {

void heat_run(heat_problem &problem) {
  double t = heat_t_lower;

  for (std::size_t i = 1; i <= heat_iters; i += 2) {
    t += problem.dt;
    heat_diffuse_rows(problem, problem.even.data(), problem.odd.data(), 0, problem.nx, t);
    t += problem.dt;
    heat_diffuse_rows(problem, problem.odd.data(), problem.even.data(), 0, problem.nx, t);
  }

  if (heat_iters % 2 != 0) {
    t += problem.dt;
    heat_diffuse_rows(problem, problem.even.data(), problem.odd.data(), 0, problem.nx, t);
  }
}

template <typename = void>
void heat_serial(benchmark::State &state) {
  run_heat(state, heat_run);
}

} // namespace

BENCH_ALL(heat_serial, serial, heat, heat)
