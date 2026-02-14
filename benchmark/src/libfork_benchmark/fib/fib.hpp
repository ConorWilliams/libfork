#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "libfork/__impl/compiler.hpp"

#include "libfork_benchmark/common.hpp"

import libfork.core;

inline constexpr int fib_test = 3;
inline constexpr int fib_base = 37;

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

[[nodiscard]]
inline auto fib_align_size(std::size_t n) -> std::size_t {
  return (n + lf::k_new_align - 1) & ~(lf::k_new_align - 1);
}
