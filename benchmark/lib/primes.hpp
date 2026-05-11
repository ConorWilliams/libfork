#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
  #include <functional>
#else
import std;
#endif

inline constexpr std::int64_t primes_test = 100'000;
inline constexpr std::int64_t primes_base = 10'000'000;

// 6k +/- 1 trial division, see https://en.wikipedia.org/wiki/Primality_test
inline constexpr auto is_prime(std::int64_t n) -> bool {
  if (n == 2 || n == 3) {
    return true;
  }
  if (n <= 1 || n % 2 == 0 || n % 3 == 0) {
    return false;
  }
  for (std::int64_t i = 5; i * i <= n; i += 6) {
    if (n % i == 0 || n % (i + 2) == 0) {
      return false;
    }
  }
  return true;
}

// Prime-counting function pi(n) reference values for the configured sizes.
inline constexpr auto primes_expected(std::int64_t n) -> std::int64_t {
  if (n == primes_test) {
    return 9592; // pi(1e5)
  }
  if (n == primes_base) {
    return 664'579; // pi(1e7)
  }
  return -1;
}

inline auto primes_count_is_correct(std::int64_t result, std::int64_t expect) -> bool {
  return expect < 0 || result == expect;
}

template <typename Fn>
void run_primes(benchmark::State &state, Fn fn) {
  std::int64_t n = state.range(0);
  std::int64_t expect = primes_expected(n);

  state.counters["n"] = static_cast<double>(n);

  lf_bench::bench(state, lf_bench::no_threads, expect, primes_count_is_correct, [n, fn]() mutable -> std::int64_t {
    benchmark::DoNotOptimize(n);
    return std::invoke(fn, n);
  });
}
