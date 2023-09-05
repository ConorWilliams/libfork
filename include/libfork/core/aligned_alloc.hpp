#ifndef DA2A13F9_F1E4_4CB1_B7C2_A9C7E6A03BDA
#define DA2A13F9_F1E4_4CB1_B7C2_A9C7E6A03BDA

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

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

namespace lf::detail {

template <typename T, typename U>
[[nodiscard]] auto r_cast(U &&expr) noexcept -> T {
  return reinterpret_cast<T>(std::forward<U>(expr)); // NOLINT
}

[[nodiscard]] inline auto aligned_alloc(std::size_t size, std::size_t alignment) -> void * {

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

/**
 * @brief Round-up n to a multiple of default new alignment.
 */
inline auto align(std::size_t const n) -> std::size_t {

  static_assert(std::has_single_bit(detail::k_new_align));

  //        k_new_align = 001000  or some other power of 2
  //    k_new_align - 1 = 000111
  // ~(k_new_align - 1) = 111000

  constexpr auto mask = ~(detail::k_new_align - 1);

  // (n + k_new_align - 1) ensures when we round-up unless n is already a multiple of k_new_align.

  return (n + detail::k_new_align - 1) & mask;
}

} // namespace lf::detail

#endif /* DA2A13F9_F1E4_4CB1_B7C2_A9C7E6A03BDA */
