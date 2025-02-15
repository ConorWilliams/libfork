// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>   // for max
#include <bit>         // for has_single_bit
#include <cstddef>     // for size_t, byte, nullptr_t
#include <cstdlib>     // for free, malloc
#include <new>         // for bad_alloc
#include <type_traits> // for is_trivially_default_constructible_v, is_trivia...
#include <utility>     // for exchange, swap
#include <version>

#include "libfork/stack.hpp"

#include "libfork/macro.hpp"   // for LF_ASSERT, LF_LOG, LF_FORCEINLINE, LF_NOINLINE
#include "libfork/utility.hpp" // for byte_cast, k_new_align, non_null, immovable

namespace {

#ifndef LF_FIBRE_INIT_SIZE
  #define LF_FIBRE_INIT_SIZE 1 // NOLINT
#endif

/**
 * @brief The initial size of a stacklet.
 *
 * Brace initialisation guarantees non-narrowing.
 */
constexpr std::size_t fibre_init_size{LF_FIBRE_INIT_SIZE};

/**
 * @brief Round size close to a multiple of the page_size.
 */
[[nodiscard]] constexpr auto round_up_to_page_size(std::size_t size) noexcept -> std::size_t {

  // Want calculate req such that:

  // req + malloc_block_est is a multiple of the page size.
  // req > size + stacklet_size

  constexpr std::size_t page_size = 4096;                           // 4 KiB on most systems.
  constexpr std::size_t malloc_meta_data_size = 6 * sizeof(void *); // An (over)estimate.

  static_assert(std::has_single_bit(page_size));

  std::size_t const minimum = size + malloc_meta_data_size;
  std::size_t const rounded = (minimum + page_size - 1) & ~(page_size - 1);
  std::size_t const request = rounded - malloc_meta_data_size;

  LF_ASSERT(minimum <= rounded);
  LF_ASSERT(rounded % page_size == 0);
  LF_ASSERT(request >= size);

  return request;
}

} // namespace

namespace lf {

// Keep stack aligned.
static_assert(sizeof(stacklet) >= detail::k_new_align);
static_assert(sizeof(stacklet) % detail::k_new_align == 0);

// Stacklet is implicit lifetime type

#ifdef __cpp_lib_is_implicit_lifetime

static_assert(std::is_implicit_lifetime_v<stacklet>);

#else

// See: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2674r0.pdf

static_assert(std::is_trivially_copy_constructible_v<stacklet>);
static_assert(std::is_trivially_destructible_v<stacklet>);

#endif

void detail::stacklet_deleter::operator()(stacklet *ptr) const noexcept {
  LF_LOG("Freeing a stacklet");
  LF_ASSERT(ptr);
  LF_ASSERT(ptr->m_prev == nullptr);
  ptr->set_next(nullptr);
  std::free(ptr); // NOLINT
}

auto make_stack() -> stack_ptr {
  return stack_ptr{stacklet::next_stacklet(fibre_init_size, nullptr)};
}

void stacklet::set_next(stacklet *new_next) noexcept {
  LF_ASSERT(is_top());
  stacklet *prev = std::exchange(m_next, new_next);
  LF_ASSERT(!prev || prev->empty());
  std::free(prev); // NOLINT
}

[[nodiscard]] auto stacklet::next_stacklet(std::size_t size, stacklet *prev) -> stacklet * {

  LF_LOG("Allocating a new stacklet");

  LF_ASSERT(prev == nullptr || prev->is_top());

  std::size_t const request = round_up_to_page_size(size + sizeof(stacklet));

  LF_ASSERT(request >= sizeof(stacklet) + size);

  stacklet *next = static_cast<stacklet *>(std::malloc(request)); // NOLINT

  if (next == nullptr) {
    LF_THROW(std::bad_alloc());
  }

  if (prev != nullptr) {
    // set_next tidies up other next.
    prev->set_next(next);
  }

  next->lo = byte_cast(next) + sizeof(stacklet);
  next->sp = next->lo;
  next->hi = byte_cast(next) + request;

  next->m_prev = prev;
  next->m_next = nullptr;

  return next;
}

/**
 * @brief Allocate `count` bytes of memory on a stacklet in the bundle.
 *
 * The memory will be aligned to a multiple of
 * `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
 *
 * Deallocate the memory with `deallocate` in a FILO manor.
 */
void stack_ptr::next_stacklet(std::size_t ext_size) {
  //
  LF_ASSERT(*this);

  stacklet *top = this->release();

  LF_ASSERT(top);

  if (top->m_next != nullptr && top->m_next->capacity() >= ext_size) {
    *this = stack_ptr{top->m_next};
  } else {
    *this = stack_ptr{stacklet::next_stacklet(std::max(2 * top->capacity(), ext_size), top)};
  }
}

/**
 * @brief Deallocate `count` bytes of memory from the current stack.
 *
 * This must be called in FILO order with `allocate`.
 */
void stack_ptr::prev_stacklet() noexcept {
  //
  LF_ASSERT(*this);
  stacklet *top = this->release();
  LF_ASSERT(top->is_top() && top->empty());

  // Always free a second order cached stacklet if it exists.
  top->set_next(nullptr);
  LF_ASSERT(top->m_prev);

  // Move to prev stacklet.
  top = top->m_prev;
  LF_ASSERT(top->m_next);

  static constexpr std::size_t k_max_growth = 8;

  // Guard against over-caching.
  if (top->m_next->capacity() > k_max_growth * top->capacity()) {
    // Free oversized stacklet.
    top->set_next(nullptr);
  }

  *this = stack_ptr{top};
}

} // namespace lf
