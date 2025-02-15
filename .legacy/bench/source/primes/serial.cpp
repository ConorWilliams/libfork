#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void primes_serial(benchmark::State &state) {

  state.counters["primes(n)"] = primes_lim;

  volatile int secret = primes_lim;
  volatile int output = 0;

  for (auto _ : state) {
    //
    std::size_t sum = 0;

    auto iota = std::ranges::views::iota(1, secret);

    for (auto &&elem : std::ranges::views::reverse(iota)) {
      sum += is_prime(elem);
    }

    output = sum;
  }
}

} // namespace

BENCHMARK(primes_serial)->UseRealTime();