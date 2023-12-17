#ifndef F7577AB3_0439_404D_9D98_072AB84FBCD0
#define F7577AB3_0439_404D_9D98_072AB84FBCD0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

#include "libfork/core/macro.hpp"

#include "libfork/core/impl/utility.hpp"

/**
 * @file fibre.hpp
 *
 * @brief Implementation of libfork's geometric segmented fibres.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A fibre is a user-space (geometric) segmented stack.
 *
 * A fibre stores the execution of a DAG from root (which may be a stolen task or true root) to suspend point.
 * A fibre is composed of fibrils, each fibril is a contiguous region of stack space stored in a double-linked
 * list. A fibre tracks the top fibril, the top fibril contains the last allocation or the fibre is empty. The
 * top fibril may have zero or one cached fibrils "ahead" of it.
 */
class fibre {

  // Want underlying malloc to allocate one-page for the first fibril.
  static constexpr std::size_t malloc_block_est = 64;
  static constexpr std::size_t fibril_size = 6 * sizeof(void *);
  static constexpr std::size_t k_init_size = 4 * impl::k_kibibyte - fibril_size - malloc_block_est;

 public:
  /**
   * @brief A fibril is a fibre fragment that contains a segment of the stack.
   *
   * A chain of fibrils looks like `R <- F1 <- F2 <- F3 <- ... <- Fn` where `R` is the root fibril.
   *
   * A fibril is allocated as a contiguous chunk of memory, the first bytes of the chunk contain the fibril
   * object. Semantically, a fibril is a dynamically sized object.
   */
  class alignas(impl::k_new_align) fibril : impl::immovable<fibril> {

    friend class fibre;

    /**
     * @brief Capacity of the current fibril's stack.
     */
    [[nodiscard]] auto capacity() const noexcept -> std::size_t {
      LF_ASSERT(m_hi - m_lo >= 0);
      return m_hi - m_lo;
    }

    /**
     * @brief Unused space on the current fibril's stack.
     */
    [[nodiscard]] auto unused() const noexcept -> std::size_t {
      LF_ASSERT(m_hi - m_sp >= 0);
      return m_hi - m_sp;
    }

    /**
     * @brief Check if fibril's stack is empty.
     */
    [[nodiscard]] auto empty() const noexcept -> bool { return m_sp == m_lo; }

    /**
     * @brief Check is this fibril is the top of a fibre.
     */
    [[nodiscard]] auto is_top() const noexcept -> bool {
      if (m_next != nullptr) {
        // Accept a single cached fibril above the top.
        return m_next->empty() && m_next->m_next == nullptr;
      }
      return true;
    }

    /**
     * @brief Set the next fibril in the chain to 'new_next'.
     *
     * This requires that this is the top fibril. If there is a cached fibril ahead of the top fibril then it
     * will be freed before being replaced with 'new_next'.
     */
    void set_next(fibril *new_next) noexcept {
      LF_ASSERT(is_top());
      std::free(std::exchange(m_next, new_next)); // NOLINT
    }

    /**
     * @brief Allocate a new fibril with a stack of size `size` and attach it to the given fibril chain.
     *
     * Requires that `prev` must be the top fibril in a chain or `nullptr`.
     */
    [[nodiscard]] LF_NOINLINE static auto next_fibril(std::size_t size, fibril *prev) -> fibril * {

      LF_LOG("allocating a new fibril");

      LF_ASSERT(prev == nullptr || prev->is_top());

      fibril *next = static_cast<fibril *>(std::malloc(sizeof(fibril) + size)); // NOLINT

      if (next == nullptr) {
        LF_THROW(std::bad_alloc());
      }

      if (prev != nullptr) {
        // Set next tidies up other next.
        prev->set_next(next);
      }

      next->m_lo = impl::byte_cast(next) + sizeof(fibril);
      next->m_sp = next->m_lo;
      next->m_hi = impl::byte_cast(next) + sizeof(fibril) + size;

      next->m_prev = prev;
      next->m_next = nullptr;

      return next;
    }

    std::byte *m_lo; ///< This fibril's stack.
    std::byte *m_sp; ///< The current position of the stack pointer in the stack.
    std::byte *m_hi; ///< The one-past-the-end address of the stack.
    fibril *m_prev;  ///< Doubly linked list (past).
    fibril *m_next;  ///< Doubly linked list (future).
  };

  // Keep stack aligned.
  static_assert(sizeof(fibril) == fibril_size);
  static_assert(sizeof(fibril) >= impl::k_new_align && sizeof(fibril) % impl::k_new_align == 0);
  // Implicit lifetime
  static_assert(std::is_trivially_default_constructible_v<fibril>);
  static_assert(std::is_trivially_destructible_v<fibril>);

  /**
   * @brief Constructs a fibre with a small empty stack.
   */
  fibre() : m_fib(fibril::next_fibril(k_init_size, nullptr)) { LF_LOG("Constructing a fibre"); }

  /**
   * @brief Construct a new fibre object taking ownership of the fibre that `frag` is a top-of.
   */
  explicit fibre(fibril *frag) noexcept : m_fib(frag) {
    LF_LOG("Constructing fibre from fibril");
    LF_ASSERT(frag && frag->is_top());
  }

  fibre(std::nullptr_t) = delete;

  fibre(fibre const &) = delete;

  auto operator=(fibre const &) -> fibre & = delete;

  /**
   * @brief Move-construct from `other` leaves `other` in the empty/default state.
   */
  fibre(fibre &&other) : fibre() { swap(*this, other); }

  /**
   * @brief Swap this and `other`.
   */
  auto operator=(fibre &&other) noexcept -> fibre & {
    swap(*this, other);
    return *this;
  }

  /**
   * @brief Swap `lhs` with `rhs.
   */
  inline friend void swap(fibre &lhs, fibre &rhs) noexcept {
    using std::swap;
    swap(lhs.m_fib, rhs.m_fib);
  }

  ~fibre() noexcept {
    LF_ASSERT(m_fib);
    LF_ASSERT(!m_fib->m_prev); // Should only be destructed at the root.
    m_fib->set_next(nullptr);  //
    std::free(m_fib);          // NOLINT
  }

  [[nodiscard]] auto empty() -> bool {
    LF_ASSERT(m_fib && m_fib->is_top());
    return m_fib->empty();
  }

  /**
   * @brief Release the underlying storage of the current fibre and re-initialize this one.
   *
   * A new fibre can be constructed from the fibril to continue the released fibre.
   */
  [[nodiscard]] auto release() -> fibril * {
    LF_LOG("Releasing fibre");
    LF_ASSERT(m_fib);
    return std::exchange(m_fib, fibril::next_fibril(k_init_size, nullptr));
  }

  /**
   * @brief Allocate `count` bytes of memory on a fibril in the bundle.
   *
   * The memory will be aligned to a multiple of `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
   *
   * Deallocate the memory with `deallocate` in a FILO manor.
   */
  [[nodiscard]] LF_FORCEINLINE auto allocate(std::size_t size) -> void * {
    //
    LF_ASSERT(m_fib && m_fib->is_top());

    // Round up to the next multiple of the alignment.
    std::size_t ext_size = (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);

    if (m_fib->unused() < ext_size) {
      if (m_fib->m_next != nullptr && m_fib->m_next->capacity() >= ext_size) {
        m_fib = m_fib->m_next;
      } else {
        m_fib = fibril::next_fibril(std::max(2 * m_fib->capacity(), ext_size), m_fib);
      }
    }

    LF_ASSERT(m_fib && m_fib->is_top());

    LF_LOG("Allocating {} bytes {}-{}", size, (void *)m_fib->m_sp, (void *)(m_fib->m_sp + ext_size));

    return std::exchange(m_fib->m_sp, m_fib->m_sp + ext_size);
  }

  /**
   * @brief Deallocate `count` bytes of memory from the current fibre.
   *
   * This must be called in FILO order with `allocate`.
   */
  LF_FORCEINLINE void deallocate(void *ptr) noexcept {

    LF_ASSERT(m_fib && m_fib->is_top());

    LF_LOG("Deallocating {}", ptr);

    m_fib->m_sp = static_cast<std::byte *>(ptr);

    if (m_fib->empty()) {
      m_fib->set_next(nullptr);
      m_fib = m_fib->m_prev == nullptr ? m_fib : m_fib->m_prev;
    }

    LF_ASSERT(m_fib && m_fib->is_top());
  }

  /**
   * @brief Get the fibril that the last allocation was on, this is non-null.
   */
  [[nodiscard]] auto top() noexcept -> fibril * {
    LF_ASSERT(m_fib && m_fib->is_top());
    return non_null(m_fib);
  }

 private:
  fibril *m_fib; ///< The allocation fibril.
};

} // namespace ext

} // namespace lf

#endif /* F7577AB3_0439_404D_9D98_072AB84FBCD0 */
