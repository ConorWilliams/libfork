#ifndef ADCDC097_184F_4172_B06E_215CC5B75A2B
#define ADCDC097_184F_4172_B06E_215CC5B75A2B

#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include <libfork.hpp>

inline auto to_sum() -> std::vector<double> {

  std::vector<double> data(1024 * 1024 * 1024 / sizeof(double) * 4);

  lf::xoshiro rng{lf::seed, std::random_device{}};

  std::uniform_real_distribution<double> dist{0, 1};

  for (auto &&elem : data) {
    elem = dist(rng);
  }

  return data;
}

inline auto get_data() -> std::pair<std::span<double>, double> {

  static auto data = to_sum();
  static auto sum = std::reduce(data.begin(), data.end());

  return {data, sum};
}

inline auto is_close(double a, double b) { return std::abs((a - b) / b) < 0.001; }

#endif /* ADCDC097_184F_4172_B06E_215CC5B75A2B */
