#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "primes.hpp"

import std;

namespace {

// Prime-counting function pi(n) reference values for the configured sizes.
constexpr auto primes_expected(std::int64_t n) -> std::int64_t {
  if (n == primes_test) {
    return 9592; // pi(1e5)
  }
  if (n == primes_base) {
    return 664'579; // pi(1e7)
  }
  return -1;
}

template <typename = void>
void primes_serial(benchmark::State &state) {

  std::int64_t lim = state.range(0);
  std::int64_t expect = primes_expected(lim);
  state.counters["n"] = static_cast<double>(lim);

  for (auto _ : state) {
    benchmark::DoNotOptimize(lim);
    std::int64_t count = 0;
    for (std::int64_t i = 1; i < lim; ++i) {
      count += is_prime(i) ? 1 : 0;
    }
    if (expect >= 0 && count != expect) {
      state.SkipWithError(std::format("incorrect prime count: {} != {}", count, expect));
      break;
    }
    benchmark::DoNotOptimize(count);
  }
}

} // namespace

BENCH_ALL(primes_serial, serial, primes, primes)
