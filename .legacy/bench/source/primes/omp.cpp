#include <iostream>
#include <ranges>
#include <thread>
#include <vector>

#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void primes_omp(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["primes(n)"] = primes_lim;
  state.counters["primes_chunk"] = primes_chunk;

  volatile int secret = primes_lim;
  volatile int output = 0;

  for (auto _ : state) {

    std::size_t sum = 0;

    auto iota = std::ranges::views::iota(1, secret);

#pragma omp parallel for num_threads(state.range(0)) reduction(+ : sum) schedule(dynamic, primes_chunk)      \
    firstprivate(iota)
    for (auto &&elem : std::ranges::views::reverse(iota)) {
      sum += is_prime(elem);
    }

    output = sum;
  }
}

} // namespace

BENCHMARK(primes_omp)->Apply(targs)->UseRealTime();
