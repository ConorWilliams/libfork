#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

#include "libfork_benchmark/common.hpp"

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

// === Shared Allocator Logic ===

inline constexpr std::size_t k_fib_align = 2 * sizeof(void *);

[[nodiscard]]
inline auto fib_align_size(std::size_t n) -> std::size_t {
  return (n + k_fib_align - 1) & ~(k_fib_align - 1);
}

inline thread_local std::byte *fib_bump_ptr = nullptr;

struct fib_bump_allocator {

  static auto operator new(std::size_t sz) -> void * {
    auto *prev = fib_bump_ptr;
    fib_bump_ptr += fib_align_size(sz);
    return prev;
  }

  static auto operator delete(void *p, [[maybe_unused]] std::size_t sz) noexcept -> void {
    fib_bump_ptr = std::bit_cast<std::byte *>(p);
  }
};
