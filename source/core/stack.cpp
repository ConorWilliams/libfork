
// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <stack>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "libfork/core/stack.hpp"

namespace lf {

namespace {

template <typename T, typename U>
auto r_cast(U &&expr) noexcept -> T {
  return reinterpret_cast<T>(std::forward<U>(expr)); // NOLINT
}

[[nodiscard]] auto aligned_alloc(std::size_t size, std::size_t alignment) -> void * {

  LF_ASSERT(size > 0);                       // Should never want to allocate no memory.
  LF_ASSERT(std::has_single_bit(alignment)); // Need power of 2 alignment.
  LF_ASSERT(size % alignment == 0);          // Size must be a multiple of alignment.

  alignment = std::max(alignment, detail::k_new_align);

  /**
   * Whatever the alignment of an allocated pointer we need to add between 0 and alignment - 1 bytes to
   * bring us to a multiple of alignment. We also need to store the allocated pointer before the returned
   * pointer so adding sizeof(void *) guarantees enough space in-front.
   */

  std::size_t offset = alignment - 1 + sizeof(void *);

  void *original_ptr = ::operator new(size + offset);

  LF_ASSERT(original_ptr);

  auto raw_address = r_cast<std::uintptr_t>(original_ptr);
  auto ret_address = (raw_address + offset) & ~(alignment - 1);

  LF_ASSERT(ret_address % alignment == 0);

  *r_cast<void **>(ret_address - sizeof(void *)) = original_ptr; // Store original pointer

  return r_cast<void *>(ret_address);
}

inline void aligned_free(void *ptr) noexcept {
  LF_ASSERT(ptr);
  ::operator delete(*r_cast<void **>(r_cast<std::uintptr_t>(ptr) - sizeof(void *)));
}

static_assert(sizeof(virtual_stack) == detail::k_virtual_stack_size, "Bad padding detected!");

inline constexpr auto k_size = detail::k_virtual_stack_size;

} // namespace

virtual_stack::virtual_stack() : stack_mem() {

  if (r_cast<std::uintptr_t>(this) % k_size != 0) {
    LF_THROW(unaligned{});
  }
  m_ptr = m_buf.data();
  m_end = m_buf.data() + m_buf.size();
}

void virtual_stack::deleter::operator()(virtual_stack *ptr) LF_STATIC_CONST noexcept {
  if constexpr (!std::is_trivially_destructible_v<virtual_stack>) {
    ptr->~virtual_stack();
  }
  aligned_free(ptr);
}

void virtual_stack::arr_deleter::operator()(virtual_stack *ptr) const noexcept {
  if constexpr (!std::is_trivially_destructible_v<virtual_stack>) {
    for (std::size_t i = 0; i < m_count; ++i) {
      ptr[i].~virtual_stack(); // NOLINT
    }
  }
  aligned_free(ptr);
}

[[nodiscard]] auto virtual_stack::make_unique() -> unique_ptr_t {
  return {new (aligned_alloc(k_size, k_size)) virtual_stack(), {}};
}

[[nodiscard]] auto virtual_stack::make_unique(std::size_t count) -> unique_arr_ptr_t {

  auto *raw = static_cast<virtual_stack *>(aligned_alloc(k_size * count, k_size));

  for (std::size_t i = 0; i < count; ++i) {
    new (raw + i) virtual_stack(); // NOLINT
  }

  return unique_arr_ptr_t{raw, arr_deleter{count}};
}

[[nodiscard]] auto virtual_stack::allocate(std::size_t const n) -> void * {

  LF_LOG("Allocating {} bytes on the stack", n);

  auto *prev = m_ptr;

  auto *next = m_ptr + align(n); // NOLINT

  if (next >= m_end) {
    LF_LOG("Virtual stack overflows");
    LF_THROW(overflow{});
  }

  m_ptr = next;

#ifndef NDEBUG
  this->m_debug.emplace_back(prev, n);
#endif

  return prev;
}

auto virtual_stack::deallocate(void *ptr, std::size_t n) noexcept -> void {
  //
  LF_LOG("Deallocating {} bytes from the stack", n);

#ifndef NDEBUG
  LF_ASSERT(!this->m_debug.empty());
  LF_ASSERT(this->m_debug.back() == std::make_pair(ptr, n));
  this->m_debug.pop_back();
#endif

  auto *prev = static_cast<std::byte *>(ptr);

  // Rudimentary check that deallocate is called in a FILO manner.
  LF_ASSERT(prev >= m_buf.data());
  LF_ASSERT(m_ptr - align(n) == prev);

  m_ptr = prev;
}

[[nodiscard]] auto virtual_stack::from_address(void *buf) -> virtual_stack::handle {
  // This utilizes the fact that a stack is aligned to N which is a power of 2.
  //
  //        N  = 001000  or some other power of 2
  //     N - 1 = 000111
  //  ~(N - 1) = 111000

  constexpr auto mask = ~(k_size - 1);

  auto as_integral = reinterpret_cast<std::uintptr_t>(buf); // NOLINT
  auto address = as_integral & mask;
  auto stack = reinterpret_cast<virtual_stack *>(address); // NOLINT

  return handle{stack};
}

} // namespace lf
