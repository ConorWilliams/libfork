#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
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
