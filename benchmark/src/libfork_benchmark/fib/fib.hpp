#pragma once

#include <cstdint>

inline constexpr int fib_test = 3;
inline constexpr int fib_base = 40;

/**
 * @brief Non-recursive Fibonacci calculation
 */
constexpr auto fib_ref(std::int64_t n) -> std::int64_t {

  if (n < 2) {
    return n;
  }

  std::int64_t prev = 0;
  std::int64_t curr = 1;

  for (std::int64_t i = 2; i <= n; ++i) {
    std::int64_t next = prev + curr;
    prev = curr;
    curr = next;
  }

  return curr;
}
