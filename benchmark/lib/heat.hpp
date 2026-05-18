#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <array>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <utility>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t heat_test = 64;
inline constexpr std::size_t heat_base = 4096;

inline constexpr std::size_t heat_y = 1024;
inline constexpr std::size_t heat_iters = 100;
inline constexpr double heat_x_lower = 0.0;
inline constexpr double heat_x_upper = 1.570796326794896558;
inline constexpr double heat_y_lower = 0.0;
inline constexpr double heat_y_upper = 1.570796326794896558;
inline constexpr double heat_t_lower = 0.0;
inline constexpr double heat_t_upper = 0.0000001;

struct heat_problem {
  std::vector<double> even;
  std::vector<double> odd;
  std::size_t nx;
  std::size_t ny;
  double dx;
  double dy;
  double dt;
  double dtdxsq;
  double dtdysq;
};

inline auto heat_initial(double x, double y) -> double {
  return std::sin(x) * std::sin(y);
}

inline auto heat_boundary_a(double, double) -> double {
  return 0.0;
}

inline auto heat_boundary_b(double x, double t) -> double {
  return std::exp(-2.0 * t) * std::sin(x);
}

inline auto heat_boundary_c(double, double) -> double {
  return 0.0;
}

inline auto heat_boundary_d(double y, double t) -> double {
  return std::exp(-2.0 * t) * std::sin(y);
}

inline auto heat_solution(double x, double y, double t) -> double {
  return std::exp(-2.0 * t) * std::sin(x) * std::sin(y);
}

constexpr auto heat_abs(double value) -> double {
  return value < 0.0 ? -value : value;
}

inline auto heat_make_problem(std::size_t nx) -> heat_problem {
  heat_problem problem{
      .even = std::vector<double>(nx * heat_y),
      .odd = std::vector<double>(nx * heat_y),
      .nx = nx,
      .ny = heat_y,
      .dx = (heat_x_upper - heat_x_lower) / static_cast<double>(nx - 1),
      .dy = (heat_y_upper - heat_y_lower) / static_cast<double>(heat_y - 1),
      .dt = (heat_t_upper - heat_t_lower) / static_cast<double>(heat_iters),
      .dtdxsq = 0.0,
      .dtdysq = 0.0,
  };

  problem.dtdxsq = problem.dt / (problem.dx * problem.dx);
  problem.dtdysq = problem.dt / (problem.dy * problem.dy);
  return problem;
}

inline void heat_init_rows(heat_problem const &problem, double *out, std::size_t row_begin, std::size_t row_end) {
  for (std::size_t i = row_begin; i < row_end; ++i) {
    double x = heat_x_lower + static_cast<double>(i) * problem.dx;
    double *row = out + i * problem.ny;

    if (i == 0) {
      for (std::size_t j = 0; j < problem.ny; ++j) {
        row[j] = heat_boundary_c(heat_y_lower + static_cast<double>(j) * problem.dy, 0.0);
      }
    } else if (i == problem.nx - 1) {
      for (std::size_t j = 0; j < problem.ny; ++j) {
        row[j] = heat_boundary_d(heat_y_lower + static_cast<double>(j) * problem.dy, 0.0);
      }
    } else {
      row[0] = heat_boundary_a(x, 0.0);
      for (std::size_t j = 1; j < problem.ny - 1; ++j) {
        row[j] = heat_initial(x, heat_y_lower + static_cast<double>(j) * problem.dy);
      }
      row[problem.ny - 1] = heat_boundary_b(x, 0.0);
    }
  }
}

inline void heat_diffuse_rows(
    heat_problem const &problem, double const *in, double *out, std::size_t row_begin, std::size_t row_end, double t) {
  for (std::size_t i = row_begin; i < row_end; ++i) {
    double x = heat_x_lower + static_cast<double>(i) * problem.dx;
    double *row = out + i * problem.ny;

    if (i == 0) {
      for (std::size_t j = 0; j < problem.ny; ++j) {
        row[j] = heat_boundary_c(heat_y_lower + static_cast<double>(j) * problem.dy, t);
      }
    } else if (i == problem.nx - 1) {
      for (std::size_t j = 0; j < problem.ny; ++j) {
        row[j] = heat_boundary_d(heat_y_lower + static_cast<double>(j) * problem.dy, t);
      }
    } else {
      row[0] = heat_boundary_a(x, t);
      for (std::size_t j = 1; j < problem.ny - 1; ++j) {
        std::size_t idx = i * problem.ny + j;
        row[j] = in[idx] + problem.dtdysq * (in[idx + 1] - 2.0 * in[idx] + in[idx - 1]) +
                 problem.dtdxsq * (in[idx + problem.ny] - 2.0 * in[idx] + in[idx - problem.ny]);
      }
      row[problem.ny - 1] = heat_boundary_b(x, t);
    }
  }
}

inline auto heat_matches(heat_problem const &problem, double const *actual) -> bool {
  double max_absolute_error = 0.0;
  double max_relative_error = 0.0;
  double mean_error = 0.0;

  std::array<std::size_t, 8> rows = {
      0,
      1,
      problem.nx / 4,
      problem.nx / 2,
      (3 * problem.nx) / 4,
      problem.nx > 2 ? problem.nx - 2 : 0,
      problem.nx - 1,
      problem.nx / 3,
  };
  std::array<std::size_t, 8> cols = {
      0,
      1,
      problem.ny / 4,
      problem.ny / 2,
      (3 * problem.ny) / 4,
      problem.ny > 2 ? problem.ny - 2 : 0,
      problem.ny - 1,
      problem.ny / 3,
  };

  for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
    std::size_t i = rows[row_idx];
    double x = heat_x_lower + static_cast<double>(i) * problem.dx;
    for (std::size_t col_idx = 0; col_idx < cols.size(); ++col_idx) {
      std::size_t j = cols[col_idx];
      double y = heat_y_lower + static_cast<double>(j) * problem.dy;
      double value = actual[i * problem.ny + j];
      double error = heat_abs(value - heat_solution(x, y, heat_t_upper));

      mean_error += error;
      if (error > max_absolute_error) {
        max_absolute_error = error;
      }
      double abs_value = heat_abs(value);
      if (abs_value > 0.0) {
        double relative_error = error / abs_value;
        if (relative_error > max_relative_error) {
          max_relative_error = relative_error;
        }
      }
    }
  }

  mean_error /= static_cast<double>(rows.size() * cols.size());
  return max_absolute_error <= 1e-10 && max_relative_error <= 1e-10 && mean_error <= 1e-10;
}

template <typename Fn>
void run_heat(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["ny"] = static_cast<double>(heat_y);
  state.counters["iters"] = static_cast<double>(heat_iters);

  heat_problem problem = heat_make_problem(n);

  lf_bench::bench(state, threads, true, [&]() -> bool {
    heat_init_rows(problem, problem.even.data(), 0, problem.nx);

    std::invoke(fn, problem);

    double const *result = (heat_iters % 2 != 0) ? problem.odd.data() : problem.even.data();
    benchmark::DoNotOptimize(result);
    return heat_matches(problem, result);
  });
}

template <typename Fn>
void run_heat(benchmark::State &state, Fn fn) {
  run_heat(state, lf_bench::no_threads, fn);
}
