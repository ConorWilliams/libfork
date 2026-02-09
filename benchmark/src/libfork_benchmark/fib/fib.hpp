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

// === Shared Allocator Logic ===

inline constexpr std::size_t k_fib_align = 2 * sizeof(void *);

[[nodiscard]]
inline auto fib_align_size(std::size_t n) -> std::size_t {
  return (n + k_fib_align - 1) & ~(k_fib_align - 1);
}

constinit inline thread_local std::byte *tls_bump_ptr = nullptr;

struct tls_bump {

  static auto operator new(std::size_t sz) -> void * {
    auto *prev = tls_bump_ptr;
    tls_bump_ptr += fib_align_size(sz);
    return prev;
  }

  static auto operator delete(void *p, [[maybe_unused]] std::size_t sz) noexcept -> void {
    tls_bump_ptr = std::bit_cast<std::byte *>(p);
  }
};

// === Shared Context Logic ===

template <lf::stack_allocator Alloc>
struct vector_ctx {

  using handle_type = lf::frame_handle<vector_ctx>;

  std::vector<handle_type> work;
  Alloc allocator;

  vector_ctx() { work.reserve(1024); }

  auto alloc() noexcept -> Alloc & { return allocator; }

  void push(handle_type handle) { work.push_back(handle); }

  auto pop() noexcept -> handle_type {
    auto handle = work.back();
    work.pop_back();
    return handle;
  }
};

template <lf::stack_allocator Alloc>
struct poly_vector_ctx final : lf::polymorphic_context<Alloc> {

  using handle_type = lf::frame_handle<lf::polymorphic_context<Alloc>>;

  std::vector<handle_type> work;

  poly_vector_ctx() { work.reserve(1024); }

  void push(handle_type handle) override { work.push_back(handle); }

  auto pop() noexcept -> handle_type override {
    auto handle = work.back();
    work.pop_back();
    return handle;
  }
};
