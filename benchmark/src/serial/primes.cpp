#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "primes.hpp"

import std;

namespace {

template <typename = void>
void primes_serial(benchmark::State &state) {
  run_primes(state, [](std::int64_t lim) {
    std::int64_t count = 0;
    for (std::int64_t i = 1; i < lim; ++i) {
      count += is_prime(i) ? 1 : 0;
    }
    return count;
  });
}

} // namespace

BENCH_ALL(primes_serial, serial, primes, primes)
