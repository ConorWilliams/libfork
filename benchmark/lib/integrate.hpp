#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
#else
import std;
#endif

inline constexpr std::int64_t integrate_test = 100;
inline constexpr std::int64_t integrate_base = 10'000;

inline constexpr double integrate_epsilon = 1.0e-9;

inline constexpr auto integrate_fn(double x) -> double { return (x * x + 1.0) * x; }

inline constexpr auto integrate_exact(double a, double b) -> double {
  auto indefinite = [](double x) { return 0.25 * x * x * (x * x + 2); };
  return indefinite(b) - indefinite(a);
}
