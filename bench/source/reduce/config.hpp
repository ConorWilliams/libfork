#ifndef ADCDC097_184F_4172_B06E_215CC5B75A2B
#define ADCDC097_184F_4172_B06E_215CC5B75A2B

#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include <libfork.hpp>

inline auto to_sum() -> std::vector<float> {

  std::vector<float> data(100'000'000);

  lf::xoshiro rng{lf::seed, std::random_device{}};

  std::uniform_real_distribution<float> dist{0, 1};

  for (auto &&elem : data) {
    elem = dist(rng);
  }

  return data;
}

inline auto is_close(double a, double b) { return std::abs((a - b) / b) < 0.001; }

#endif /* ADCDC097_184F_4172_B06E_215CC5B75A2B */
