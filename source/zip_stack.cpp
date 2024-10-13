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

#include "libfork/detail/zip_stack.hpp"

#include "libfork/detail/macro.hpp"   // for LF_ASSERT, LF_LOG, LF_FORCEINLINE, LF_NOINLINE
#include "libfork/detail/utility.hpp" // for byte_cast, k_new_align, non_null, immovable

namespace {

/**
 * @brief Round size (up) close to a multiple of the page_size.
 *
 * Returns a new size that is at least `size` bytes and is close to a multiple of the page size.
 */
[[nodiscard]] constexpr auto round_up_to_page_size(std::size_t size) noexcept -> std::size_t {

  // Want to calculate req such that:

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

namespace detail {

void root_stacklet::pop_stacklet() noexcept {

  LF_ASSERT(m_top, "m_top should never be null");
  LF_ASSERT(!m_top->is_root(), "Root stacklet cannot pop itself");

  auto *prev_top = std::exchange(m_top, m_top->prev);

  std::destroy_at(prev_top);

  std::free(prev_top); // NOLINT
}

auto root_stacklet::next_zip_stacklet(std::size_t count, link_stacklet *prev) -> link_stacklet * {}

} // namespace detail

void zip_stack::pop_stacklet() noexcept {

  LF_ASSERT(empty(), "Precondition failed: top stacklet is not empty");

  if (!m_ctrl) {
    return;
  }

  m_ctrl->pop_stacklet();
}

void zip_stack::next_stacklet(std::size_t count) {

  // If the current stacklet is empty we replace it.

  if (empty()) {
    // If top == root we need a new root, else we can just allocate,
    pop_stacklet();
  }

  constexpr std::size_t growth_factor = 2;

  std::size_t const cap = capacity();

  LF_ASSERT(cap % 2 == 0, "Capacity is not even", capacity());

  count = std::max(count, cap * growth_factor + cap / 2);

  auto *next = detail::root_stacklet::next_zip_stacklet(count, m_ctrl.get());

  if (!m_ctrl) {
    // We just allocated a root stacklet, take ownership.
    m_ctrl = std::unique_ptr<root_t>{static_cast<root_t *>(next)};
  }

  LF_ASSERT(m_ctrl, "Post condition failed: zip-stack is null");
  LF_ASSERT(unused() >= count, "Post condition failed: insufficient space on stacklet");
}

} // namespace lf