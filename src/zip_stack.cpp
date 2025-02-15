// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm> // for max
#include <bit>       // for has_single_bit
#include <cstddef>   // for size_t, byte, nullptr_t
#include <cstdlib>   // for free, malloc
#include <memory>
#include <new>         // for bad_alloc
#include <type_traits> // for is_trivially_default_constructible_v, is_trivia...
#include <utility>     // for exchange, swap
#include <version>

#include "libfork/zip_stack.hpp"

#include "libfork/macro.hpp"   // for LF_ASSERT, LF_LOG, LF_FORCEINLINE, LF_NOINLINE
#include "libfork/utility.hpp" // for byte_cast, k_new_align, non_null, immovable

namespace {

/**
 * @brief Round size (up) close to a multiple of the page_size.
 *
 * Returns a new size that is at least `size` bytes and is close to a multiple
 * of the page size.
 */
[[nodiscard]] constexpr auto round_up_to_page_size(std::size_t size) noexcept -> std::size_t {

  // Want to calculate req such that:

  // req + malloc_block_est is a multiple of the page size.
  // req > size + stacklet_size

  constexpr std::size_t page_size = 4096;                           // 4 KiB on most systems.
  constexpr std::size_t malloc_meta_data_size = 4 * sizeof(void *); // (over)estimate.

  static_assert(std::has_single_bit(page_size));

  std::size_t const minimum = size + malloc_meta_data_size;
  std::size_t const rounded = (minimum + page_size - 1) & ~(page_size - 1);
  std::size_t const request = rounded - malloc_meta_data_size;

  LF_ASSERT(minimum <= rounded);
  LF_ASSERT(rounded % page_size == 0);
  LF_ASSERT(request >= size);

  return request;
}

struct Alloc {
  void *ptr;
  std::byte *lo;
  std::byte *hi;
};

auto alloc(std::size_t count, std::size_t plus) -> Alloc {

  std::size_t tot = round_up_to_page_size(plus + count);

  void *ptr = std::malloc(tot);

  if (ptr == nullptr) {
    LF_THROW(std::bad_alloc());
  }

  std::byte *lo = lf::detail::as_byte_ptr(ptr) + sizeof(plus);
  std::byte *hi = lf::detail::as_byte_ptr(ptr) + tot;

  return {ptr, lo, hi};
}

} // namespace

namespace lf {

auto root(std::size_t count) -> stack * {
  auto [ptr, lo, hi] = alloc(count, sizeof(stack));
  return new (ptr) stack{lo, hi};
}

auto stack::push(std::size_t count) -> stacklet * {
  auto [ptr, lo, hi] = alloc(count, sizeof(stacklet));
  stacklet *new_stacklet = new (ptr) stacklet{lo, hi, m_top};
  m_top = new_stacklet;
  return new_stacklet;
}

void stack::pop() noexcept {
  LF_ASSERT(m_top, "m_top should never be null");
  LF_ASSERT(!m_top->is_root(), "Root stacklet cannot pop itself");

  auto *prev_top = std::exchange(m_top, m_top->prev);
  std::destroy_at(prev_top);
  std::free(prev_top);
}

void stack::handle::deleter::operator()(stack *ptr) noexcept {
  std::destroy_at(ptr);
  std::free(ptr);
}

void stack::handle::pop_stacklet(void *ptr) noexcept {

  m_root->pop();

  m_lo = m_root->m_top->lo;
  m_sp = detail::as_byte_ptr(ptr);
  m_hi = m_root->m_top->hi;
}

void stack::handle::push_stacklet(std::size_t count) {

  // If the current stacklet is empty we remove it so that the previous
  // allocation is always on the previous stacklet.

  if (!m_root) {
    *this = stack::handle{root(count)};
    return;
  }

  // TODO: design tests to hit all of these

  if (empty()) {
    if (m_root->m_top->is_root()) {
      *this = stack::handle{root(count)};
      return;
    }
    m_root->pop();
  }

  constexpr std::size_t growth_factor = 2;
  std::size_t const cap = capacity();

  // Next stacklet should support undeflow of: cap / 2
  // Undeflow must be multiple of k_new_align to maintain alignment.
  auto underflow = round(cap / 2);
  count = std::max(count, underflow + cap * growth_factor);

  stacklet *next = m_root->push(count);

  m_lo = next->lo;
  m_sp = next->lo + underflow;
  m_hi = next->hi;

  LF_ASSERT(m_root, "Post condition failed: zip-stack is null");
  LF_ASSERT(unused() >= count, "Post condition failed: insufficient space on stacklet");
}

} // namespace lf
