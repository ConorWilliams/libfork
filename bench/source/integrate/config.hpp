#ifndef BC8182D4_CB6F_446F_80B2_FFA0DC4A9C61
#define BC8182D4_CB6F_446F_80B2_FFA0DC4A9C61

#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

inline constexpr int n = 10000;
inline constexpr double epsilon = 1.0e-7;

inline constexpr auto fn(double x) -> double { return (x * x + 1.0) * x; }

inline constexpr auto integral_fn(double a, double b) -> double {

  constexpr auto indefinite = [](double x) {
    return 0.25 * x * x * (x * x + 2);
  };

  return indefinite(b) - indefinite(a);
}

#endif /* BC8182D4_CB6F_446F_80B2_FFA0DC4A9C61 */
