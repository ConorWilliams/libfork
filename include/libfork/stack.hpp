#ifndef F7577AB3_0439_404D_9D98_072AB84FBCD0
#define F7577AB3_0439_404D_9D98_072AB84FBCD0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstddef> // for size_t, byte, nullptr_t
#include <cstdlib> // for free, malloc
#include <memory>
#include <utility> // for exchange, swap
#include <version>

#include "libfork/macro.hpp"   // for LF_ASSERT, LF_LOG, LF_FORCEINLINE, LF_NOINLINE
#include "libfork/utility.hpp" // for byte_cast, k_new_align, non_null, immovable

/**
 * @file stack.hpp
 *
 * @brief Implementation of libfork's geometric segmented stacks.
 */

namespace lf {

class stack_ptr;

class stacklet;

/**
 * @brief Allocate a stacklet and return an owning stack_ptr to it.
 */
[[nodiscard]] auto make_stack() -> stack_ptr;

namespace detail {

struct stacklet_deleter {
  void operator()(stacklet *ptr) const noexcept;
};

/**
 * @brief A stack span is a view of a stacklet's stack.
 */
struct stack_span {
  /**
   * @brief This stacklet's stack.
   */
  std::byte *lo = nullptr;
  /**
   * @brief The current position of the stack pointer in the stack.
   */
  std::byte *sp = nullptr;
  /**
   * @brief The one-past-the-end address of the stack.
   */
  std::byte *hi = nullptr;
  /**
   * @brief Capacity of the current stacklet's stack.
   */
  [[nodiscard]] auto capacity() const noexcept -> std::size_t {
    return checked_cast<std::size_t>(hi - lo);
  }
  /**
   * @brief Unused space on the current stacklet's stack.
   */
  [[nodiscard]] auto unused() const noexcept -> std::size_t {
    return checked_cast<std::size_t>(hi - sp);
  }
  /**
   * @brief Check if stacklet's stack is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool { return sp == lo; }
};

} // namespace detail

/**
 * @brief A stacklet is a stack fragment that contains a segment of the stack.
 *
 * A chain of stacklets looks like `R <- F1 <- F2 <- F3 <- ... <- Fn` where `R`
 * is the root stacklet.
 *
 * A stacklet is allocated as a contiguous chunk of memory, the first bytes of
 * the chunk contain the stacklet object. Semantically, a stacklet is a
 * dynamically sized object.
 *
 */
class alignas(detail::k_new_align) stacklet : public detail::stack_span {
 public:
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

 private:
  /**
   * @brief Try and guard against people making instances of this class.
   */
  stacklet() = default;

  friend class stack_ptr;

  friend struct detail::stacklet_deleter;

  friend auto make_stack() -> stack_ptr;

  /**
   * @brief Set the next stacklet in the chain to 'new_next'.
   *
   * This requires that this is the top stacklet. If there is a cached stacklet
   * ahead of the top stacklet then it will be freed before being replaced with
   * 'new_next'.
   */
  void set_next(stacklet *new_next) noexcept;

  /**
   * @brief Allocate a new stacklet with a stack of size of at least `size` and
   * attach it to the given stacklet chain.
   *
   * Requires that `prev` must be the top stacklet in a chain or `nullptr`. This
   * will round size up to a multiple of the alignment/page size.
   *
   * Returns a pointer to the newly allocated stacklet.
   */
  [[nodiscard]] static auto next_stacklet(std::size_t size, stacklet *prev) -> stacklet *;

  /**
   * @brief Doubly linked list (past).
   */
  stacklet *m_prev = nullptr;
  /**
   * @brief Doubly linked list (future).
   */
  stacklet *m_next = nullptr;
};

/**
 * @brief A stack is a user-space (geometric) segmented program stack.
 *
 * A stack stores the execution of a DAG from root (which may be a stolen task
 * or true root) to suspend point. A stack is composed of stacklets, each
 * stacklet is a contiguous region of stack space stored in a double-linked
 * list. A stack tracks the top stacklet, the top stacklet contains the last
 * allocation or the stack is empty. The top stacklet may have zero or one
 * cached stacklets "ahead" of it. A stacklet may not be destructed until it is
 * null or empty.
 */
class stack_ptr : std::unique_ptr<stacklet, detail::stacklet_deleter> {

  static constexpr std::size_t k_new_align = detail::k_new_align;

  using base_ptr = std::unique_ptr<stacklet, detail::stacklet_deleter>;

  /**
   * @brief A cache of the top stacklet's stack_span.
   */
  detail::stack_span m_cache;

  /**
   * @brief Allocate a stacklet large enough to hold `size` bytes and attach it
   * to the current stack.
   */
  void next_stacklet(std::size_t ext_size);

  /**
   * @brief Pop the top stacklet and cache it.
   */
  void prev_stacklet() noexcept;

 public:
  /**
   * @brief Construct a null stack-pointer.
   */
  stack_ptr() = default;

  /**
   * @brief Construct a new stack object taking ownership of the stack that
   * `frag` is a top-of.
   *
   * This stacklet must have been released by its previous owner.
   */
  explicit stack_ptr(stacklet *frag) noexcept
      : base_ptr(frag),
        m_cache{.lo = frag->lo, .sp = frag->sp, .hi = frag->hi} {
    LF_LOG("stack_ptr from stacklet");
  }

  /**
   * @brief Test if the stack-pointer is not null.
   */
  using base_ptr::operator bool;

  /**
   * @brief Get a non-owning view of stacklet that the last allocation was on.
   *
   * This can be null if the stack_ptr is null.
   */
  [[nodiscard]] auto top() const noexcept -> stacklet * { return this->get(); }

  /**
   * @brief Release ownership of the stacklet and return a pointer to it.
   */
  [[nodiscard]] auto release() noexcept -> stacklet * {
    if (*this) {
      top()->sp = m_cache.sp;
    }
    return base_ptr::release();
  }

  // TODO: if empty is rare then move to .cpp

  /**
   * @brief Test if the stack is empty (has no allocations) or is null.
   */
  [[nodiscard]] auto empty() const noexcept -> bool {

    stacklet const *stk = top();

    if (stk == nullptr) {
      return true;
    }

    return m_cache.empty() && stk->m_prev == nullptr;
  }

  /**
   * @brief Allocate `count` bytes of memory on a stacklet in the bundle.
   *
   * This has the precondition that this stack_ptr is not null.
   *
   * The memory will be aligned to a multiple of
   * `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
   *
   * Deallocate the memory with `deallocate` in a FILO manor.
   */
  [[nodiscard]] auto allocate(std::size_t size) -> void * {

    LF_ASSERT(*this);

    // Round up to the next multiple of the alignment.
    std::size_t const ext_size = (size + k_new_align - 1) & ~(k_new_align - 1);

    if (m_cache.unused() < ext_size) {
      next_stacklet(ext_size);
    }

    return std::exchange(m_cache.sp, m_cache.sp + ext_size); // NOLINT
  }

  /**
   * @brief Deallocate `count` bytes of memory from the current stack.
   *
   * This has the precondition that this stack_ptr is not null.
   *
   * This must be called in FILO order with `allocate`.
   */
  void deallocate(void *ptr) noexcept {

    LF_ASSERT(*this);

    if (m_cache.empty()) {
      prev_stacklet();
    }

    LF_ASSERT(!m_cache.empty());

    auto *next_sp = static_cast<std::byte *>(ptr);

    LF_ASSERT(m_cache.lo <= next_sp && next_sp <= m_cache.sp);

    m_cache.sp = next_sp;
  }
};

} // namespace lf

#endif /* F7577AB3_0439_404D_9D98_072AB84FBCD0 */
