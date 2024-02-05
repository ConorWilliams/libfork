#ifndef F7577AB3_0439_404D_9D98_072AB84FBCD0
#define F7577AB3_0439_404D_9D98_072AB84FBCD0

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

#include "libfork/core/impl/utility.hpp" // for byte_cast, k_new_align, non_null, immovable
#include "libfork/core/macro.hpp"        // for LF_ASSERT, LF_LOG, LF_FORCEINLINE, LF_NOINLINE

/**
 * @file stack.hpp
 *
 * @brief Implementation of libfork's geometric segmented stacks.
 */

#ifndef LF_FIBRE_INIT_SIZE
  /**
   * @brief The initial size for a stack (in bytes).
   *
   * All stacklets will be round up to a multiple of the page size.
   */
  #define LF_FIBRE_INIT_SIZE 1
#endif

static_assert(LF_FIBRE_INIT_SIZE > 0, "Stacks must have a positive size");

namespace lf::impl {

/**
 * @brief Round size close to a multiple of the page_size.
 */
[[nodiscard]] inline constexpr auto round_up_to_page_size(std::size_t size) noexcept -> std::size_t {

  // Want calculate req such that:

  // req + malloc_block_est is a multiple of the page size.
  // req > size + stacklet_size

  std::size_t constexpr page_size = 4096;                           // 4 KiB on most systems.
  std::size_t constexpr malloc_meta_data_size = 6 * sizeof(void *); // An (over)estimate.

  static_assert(std::has_single_bit(page_size));

  std::size_t minimum = size + malloc_meta_data_size;
  std::size_t rounded = (minimum + page_size - 1) & ~(page_size - 1);
  std::size_t request = rounded - malloc_meta_data_size;

  LF_ASSERT(minimum <= rounded);
  LF_ASSERT(rounded % page_size == 0);
  LF_ASSERT(request >= size);

  return request;
}

/**
 * @brief A stack is a user-space (geometric) segmented program stack.
 *
 * A stack stores the execution of a DAG from root (which may be a stolen task or true root) to suspend
 * point. A stack is composed of stacklets, each stacklet is a contiguous region of stack space stored in a
 * double-linked list. A stack tracks the top stacklet, the top stacklet contains the last allocation or the
 * stack is empty. The top stacklet may have zero or one cached stacklets "ahead" of it.
 */
class stack {

 public:
  /**
   * @brief A stacklet is a stack fragment that contains a segment of the stack.
   *
   * A chain of stacklets looks like `R <- F1 <- F2 <- F3 <- ... <- Fn` where `R` is the root stacklet.
   *
   * A stacklet is allocated as a contiguous chunk of memory, the first bytes of the chunk contain the
   * stacklet object. Semantically, a stacklet is a dynamically sized object.
   *
   * Each stacklet also contains an exception pointer and atomic flag which stores exceptions thrown by
   * children.
   */
  class alignas(impl::k_new_align) stacklet : impl::immovable<stacklet> {

    friend class stack;

    /**
     * @brief Capacity of the current stacklet's stack.
     */
    [[nodiscard]] auto capacity() const noexcept -> std::size_t {
      LF_ASSERT(m_hi - m_lo >= 0);
      return m_hi - m_lo;
    }

    /**
     * @brief Unused space on the current stacklet's stack.
     */
    [[nodiscard]] auto unused() const noexcept -> std::size_t {
      LF_ASSERT(m_hi - m_sp >= 0);
      return m_hi - m_sp;
    }

    /**
     * @brief Check if stacklet's stack is empty.
     */
    [[nodiscard]] auto empty() const noexcept -> bool { return m_sp == m_lo; }

    /**
     * @brief Check is this stacklet is the top of a stack.
     */
    [[nodiscard]] auto is_top() const noexcept -> bool {
      if (m_next != nullptr) {
        // Accept a single cached stacklet above the top.
        return m_next->empty() && m_next->m_next == nullptr;
      }
      return true;
    }

    /**
     * @brief Set the next stacklet in the chain to 'new_next'.
     *
     * This requires that this is the top stacklet. If there is a cached stacklet ahead of the top stacklet
     * then it will be freed before being replaced with 'new_next'.
     */
    void set_next(stacklet *new_next) noexcept {
      LF_ASSERT(is_top());
      std::free(std::exchange(m_next, new_next)); // NOLINT
    }

    /**
     * @brief Allocate a new stacklet with a stack of size of at least`size` and attach it to the given
     * stacklet chain.
     *
     * Requires that `prev` must be the top stacklet in a chain or `nullptr`. This will round size up to
     */
    [[nodiscard]] LF_NOINLINE static auto next_stacklet(std::size_t size, stacklet *prev) -> stacklet * {

      LF_LOG("allocating a new stacklet");

      LF_ASSERT(prev == nullptr || prev->is_top());

      std::size_t request = impl::round_up_to_page_size(size + sizeof(stacklet));

      LF_ASSERT(request >= sizeof(stacklet) + size);

      stacklet *next = static_cast<stacklet *>(std::malloc(request)); // NOLINT

      if (next == nullptr) {
        LF_THROW(std::bad_alloc());
      }

      if (prev != nullptr) {
        // Set next tidies up other next.
        prev->set_next(next);
      }

      next->m_lo = impl::byte_cast(next) + sizeof(stacklet);
      next->m_sp = next->m_lo;
      next->m_hi = impl::byte_cast(next) + request;

      next->m_prev = prev;
      next->m_next = nullptr;

      return next;
    }

    /**
     * @brief Allocate an initial stacklet.
     */
    [[nodiscard]] static auto next_stacklet() -> stacklet * {
      return stacklet::next_stacklet(LF_FIBRE_INIT_SIZE, nullptr);
    }

    /**
     * @brief This stacklet's stack.
     */
    std::byte *m_lo;
    /**
     * @brief The current position of the stack pointer in the stack.
     */
    std::byte *m_sp;
    /**
     * @brief The one-past-the-end address of the stack.
     */
    std::byte *m_hi;
    /**
     * @brief Doubly linked list (past).
     */
    stacklet *m_prev;
    /**
     * @brief Doubly linked list (future).
     */
    stacklet *m_next;
  };

  // Keep stack aligned.
  static_assert(sizeof(stacklet) >= impl::k_new_align);
  static_assert(sizeof(stacklet) % impl::k_new_align == 0);
  // Stacklet is implicit lifetime type
  static_assert(std::is_trivially_default_constructible_v<stacklet>);
  static_assert(std::is_trivially_destructible_v<stacklet>);

  /**
   * @brief Constructs a stack with a small empty stack.
   */
  stack() : m_fib(stacklet::next_stacklet()) { LF_LOG("Constructing a stack"); }

  /**
   * @brief Construct a new stack object taking ownership of the stack that `frag` is a top-of.
   */
  explicit stack(stacklet *frag) noexcept : m_fib(frag) {
    LF_LOG("Constructing stack from stacklet");
    LF_ASSERT(frag && frag->is_top());
  }

  stack(std::nullptr_t) = delete;

  stack(stack const &) = delete;

  auto operator=(stack const &) -> stack & = delete;

  /**
   * @brief Move-construct from `other` leaves `other` in the empty/default state.
   */
  stack(stack &&other) : stack() { swap(*this, other); }

  /**
   * @brief Swap this and `other`.
   */
  auto operator=(stack &&other) noexcept -> stack & {
    swap(*this, other);
    return *this;
  }

  /**
   * @brief Swap `lhs` with `rhs.
   */
  inline friend void swap(stack &lhs, stack &rhs) noexcept {
    using std::swap;
    swap(lhs.m_fib, rhs.m_fib);
  }

  /**
   * @brief Destroy the stack object.
   */
  ~stack() noexcept {
    LF_ASSERT(m_fib);
    LF_ASSERT(!m_fib->m_prev); // Should only be destructed at the root.
    m_fib->set_next(nullptr);  // Free a cached stacklet.
    std::free(m_fib);          // NOLINT
  }

  /**
   * @brief Test if the stack is empty (has no allocations).
   */
  [[nodiscard]] auto empty() -> bool {
    LF_ASSERT(m_fib && m_fib->is_top());
    return m_fib->empty() && m_fib->m_prev == nullptr;
  }

  /**
   * @brief Release the underlying storage of the current stack and re-initialize this one.
   *
   * A new stack can be constructed from the stacklet to continue the released stack.
   */
  [[nodiscard]] auto release() -> stacklet * {
    LF_LOG("Releasing stack");
    LF_ASSERT(m_fib);
    return std::exchange(m_fib, stacklet::next_stacklet());
  }

  /**
   * @brief Allocate `count` bytes of memory on a stacklet in the bundle.
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
        m_fib = stacklet::next_stacklet(std::max(2 * m_fib->capacity(), ext_size), m_fib);
      }
    }

    LF_ASSERT(m_fib && m_fib->is_top());

    LF_LOG("Allocating {} bytes {}-{}", size, (void *)m_fib->m_sp, (void *)(m_fib->m_sp + ext_size));

    return std::exchange(m_fib->m_sp, m_fib->m_sp + ext_size);
  }

  /**
   * @brief Deallocate `count` bytes of memory from the current stack.
   *
   * This must be called in FILO order with `allocate`.
   */
  LF_FORCEINLINE void deallocate(void *ptr) noexcept {

    LF_ASSERT(m_fib && m_fib->is_top());

    LF_LOG("Deallocating {}", ptr);

    m_fib->m_sp = static_cast<std::byte *>(ptr);

    if (m_fib->empty()) {

      if (m_fib->m_prev != nullptr) {
        // Always free a second order cached stacklet if it exists.
        m_fib->set_next(nullptr);
        // Move to prev stacklet.
        m_fib = m_fib->m_prev;
      }

      LF_ASSERT(m_fib);

      // Guard against over-caching.
      if (m_fib->m_next != nullptr) {
        if (m_fib->m_next->capacity() > 8 * m_fib->capacity()) {
          // Free oversized stacklet.
          m_fib->set_next(nullptr);
        }
      }
    }

    LF_ASSERT(m_fib && m_fib->is_top());
  }

  /**
   * @brief Get the stacklet that the last allocation was on, this is non-null.
   */
  [[nodiscard]] auto top() noexcept -> stacklet * {
    LF_ASSERT(m_fib && m_fib->is_top());
    return non_null(m_fib);
  }

 private:
  /**
   * @brief The allocation stacklet.
   */
  stacklet *m_fib;
};

} // namespace lf::impl

#endif /* F7577AB3_0439_404D_9D98_072AB84FBCD0 */
