#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

constexpr auto count_primes = [](auto &&range) -> int {
  //
  int count = 0;

  // std::cerr << "pika" << std::endl;

  for (auto &&i : range) {
    count = count + is_prime(i);
  }

  // std::cerr << "pika" << std::endl;

  return count;
};

void primes_serial(benchmark::State &state) {

  state.counters["primes(n)"] = primes_lim;

  volatile int secret = primes_lim;
  volatile int output = 0;

  for (auto _ : state) {
    output = count_primes(std::ranges::views::iota(1, secret));
  }
}

} // namespace

BENCHMARK(primes_serial)->UseRealTime();