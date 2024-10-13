#ifndef C4F946B7_9F01_49F3_B8F7_B9235E901BD7
#define C4F946B7_9F01_49F3_B8F7_B9235E901BD7

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "libfork/detail/macro.hpp"
#include "libfork/detail/stack.hpp"
#include "libfork/detail/utility.hpp"

/**
 * @file zip_stack.hpp
 *
 * @brief ...
 *
 *  |<-------------------------------
 *  | ^         ^         ^         ^
 *  | |         |         |         |
 * [R L...] <- [L...] <- [L...] <- [L...]
 *  |                               ^
 *  v                               |
 *  ------------------------------->|
 *
 * Weak has ptr to root, can reclaim by fetching cache.
 *
 * Strong has ptr to root and stack_span:
 *  - Alloc fast-path, bump stack_span
 *  - Dealloc fast-path, bump stack_span
 *  - Alloc slow path, (del top if empty), alloc new stacklet (maybe a new root)
 *  - Dealloc slow path, del top, move top to prev stacklet
 *
 * All the stacklets of offset from each other to introduce hysteresis.
 */

namespace lf {

// Forward decl
class zip_stack;

// Forward decl
class weak_zip_stack;

namespace detail {

/**
 * @brief A region of memory that is part of a zip-stack.
 */
struct alignas(k_new_align) link_stacklet {

  auto is_root() const noexcept -> bool { return prev != nullptr; }

  std::byte *lo; ///< The start of the stacklet's stack.
  std::byte *hi; ///< The end of the stacklet's stack.

  link_stacklet *prev; ///< Singly linked list (past), null if root.
};

struct allocation_record {
  void *ptr = nullptr;   ///< The start of the allocation.
  std::size_t count = 0; ///< The size of the allocation (bytes).
};

struct root_debug {
  std::vector<allocation_record> allocations; ///< Order of allocations.
  int weak_count = 0;                         ///< Number of weak pointers.
  bool owned = true;                          ///< If a zip_stack exists then this is owned.
};

class root_stacklet : link_stacklet {
 public:
  /**
   * @brief Construct a new root stacklet object.
   *
   * @param lo The start of the stacklet's stack.
   * @param hi The end of the stacklet's stack.
   */
  root_stacklet(std::byte *lo, std::byte *hi) noexcept
      : link_stacklet{.lo = lo, .hi = hi, .prev = nullptr},
        m_sp{lo},
        m_top{this} {}

  /**
   * @brief Make a weak pointer to the root stacklet.
   *
   * The weak pointer must call `drop_weak` at the end of its lifetime
   * or make a call to `acquire`.
   *
   * This requires that the root stacklet is owned.
   */
  void make_weak() {
    LF_ASSERT(m_debug.owned, "Root stacklet is not owned");
    LF_ASSERT(++m_debug.weak_count > 0, "Weak count is negative");
  }

  /**
   * @brief Release a weak handle to this root stacklet.
   *
   * This requires that at-least one weak handle exists.
   */
  void drop_weak() { LF_ASSERT(--m_debug.weak_count >= 0, "No weak handle to drop"); }

  /**
   * @brief Convert a weak handle to an owning handle.
   *
   * This requires that the root stacklet is unowned.
   */
  void weak_to_strong() {
    drop_weak();
    LF_ASSERT(!std::exchange(m_debug.owned, true), "Acquiring an owned root stacklet");
  }

  /**
   * @brief Release an owning handle to this root stacklet.
   *
   * This requires that the root stacklet is owned by the zip_stack.
   *
   * @param sp The stack pointer of the top stacklet at the time of release.
   */
  void release(std::byte *sp) {
    LF_ASSERT(std::exchange(m_debug.owned, true), "Releasing an unowned root stacklet");
    m_sp = sp;
  }

  /**
   * @brief Free the top stacklet.
   *
   * This requires that the root stacklet is not the top stacklet.
   */
  void pop_stacklet();

  /**
   * @brief Allocate a new zip-stacklet with a stack of size of at least
   * `count` bytes and attach it to the given zip-stacklet chain.
   *
   * If `prev` is null then the new stacklet will be the root of the zip-stack.
   */
  friend auto push_stacklet(root_stacklet *prev, std::size_t count) -> link_stacklet *;

 private:
  std::byte *m_sp;      ///< Cache of the stack pointer (for release).
  link_stacklet *m_top; ///< The top stacklet of the zip-stack.

#ifndef NDEBUG
  root_debug m_debug; ///< Debug information (only stored in the root).
#endif
};

} // namespace detail

/**
 * @brief A weak handle to a zip-stack.
 *
 * Weak handles can be obtained from a zip-stack and stored in a region of memory allocated
 * on the zip-stack. Hence, their lifetime should be entirely nested withing the lifetime of
 * allocation and thus also nested within the lifetime of the zip-stack they refer to. Weak
 * zip-stack pointers remains valid throughout calls to release/acquire.
 */
class weak_zip_stack {
 public:
  /**
   * @brief Convert to an owning handle to the underlying zip-stack.
   *
   * This consumes the weak handle and leaves it in a null state.
   *
   * Requires the underlying zip-stack has been released.
   */
  [[nodiscard]] auto acquire() && noexcept -> zip_stack;

 private:
  friend class zip_stack;
  friend class detail::root_stacklet;

  using root_t = detail::root_stacklet;

  /**
   * @brief Construct a new weak zip-stack object.
   */
  explicit weak_zip_stack(root_t *ptr) noexcept : m_ctrl{ptr} {
    LF_ASSERT(ptr, "Weak zip-stack pointer is null");
  }

  /**
   * @brief Decrement the weak reference count in debug mode.
   */
  struct deleter {
    void operator()(root_t *root) noexcept { root->drop_weak(); }
  };

  std::unique_ptr<root_t, deleter> m_ctrl;
};

namespace detail {

auto root_stacklet::make_weak() noexcept -> weak_zip_stack {
#ifndef NDEBUG
  m_debug.weak_count++;
#endif
  return weak_zip_stack{this};
}

} // namespace detail

/**
 * @brief An owning handle to a zip-stack.
 */
class zip_stack {
 public:
  /**
   * @brief Construct a new null zip-stack
   */
  zip_stack() = default;

  /**
   * @brief Check if the zip-stack is null.
   */
  [[nodiscard]] operator bool() const noexcept -> bool { return static_cast<bool>(m_ctrl); }

  /**
   * @brief Check if a weak handle points to this zip-stack.
   */
  [[nodiscard]] auto operator==(weak_zip_stack const &other) const noexcept -> bool {
    return m_ctrl.get() == other.m_ctrl.get();
  }

  /**
   * @brief Construct a weak handle to this zip-stack.
   *
   * Requires that this zip-stack is non-null.
   */
  [[nodiscard]] auto weak() const noexcept -> weak_zip_stack {

    LF_ASSERT(m_ctrl, "Cannot make a weak pointer to a null zip-stack");

    return m_ctrl->make_weak();
  }

  /**
   * @brief Release ownership of the zip-stack and return a weak handle to it.
   *
   * This requires that the zip-stack is non-null.
   *
   * After this operation the zip-stack is null, A weak handle must be acquired
   * at some point to destroy the zip-stack.
   */
  auto release() && {

    LF_ASSERT(m_ctrl, "Cannot release a null zip-stack");

    return detail::root_stacklet::release(m_ctrl.release(), m_sp);
  }

  /**
   * @brief Allocate `count` bytes of memory on the zip-stack.
   *
   * The zip stack may be null.
   *
   * The memory will be aligned to a multiple of `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
   *
   * Deallocate the memory with `deallocate` in a FILO manor.
   */
  [[nodiscard]] auto allocate(std::size_t count) -> void * {

    count = round(count);

    if (unused() < count) {
      next_stacklet(count);
    }

    return std::exchange(m_sp, m_sp + count); // NOLINT
  }

  /**
   * @brief Deallocate memory allocated on the zip-stack.
   *
   * The zip stack must not be null.
   *
   * The memory must have been allocated with `allocate` and must be deallocated in a FILO manor.
   */
  void deallocate(void *ptr, std::size_t) noexcept;

 private:
  /**
   * @brief Add a new stacklet to the zip-stack.
   */
  void next_stacklet(std::size_t count);

  /**
   * @brief Remove the top stacklet, a noop if null.
   */
  void pop_stacklet() noexcept;

  /**
   * @brief Round to a multiple of '__STDCPP_DEFAULT_NEW_ALIGNMENT__'.
   */
  static constexpr auto round(std::size_t n) -> std::size_t {
    return (n + detail::k_new_align - 1) & ~(detail::k_new_align - 1);
  }

  /**
   * Allow zip_stack_control_block to construct zip_stack objects.
   */
  friend struct detail::link_stacklet;

  using root_t = detail::root_stacklet;

  explicit zip_stack(root_t *ptr, std::byte *lo, std::byte *sp, std::byte *hi) noexcept
      : m_ctrl{ptr},
        m_lo{lo},
        m_sp{sp},
        m_hi{hi} {}

  /**
   * @brief Properly delete the paired allocation.
   */
  struct deleter {
    void operator()(root_t *) noexcept;
  };

  /**
   * @brief The control block of the zip-stack.
   */
  std::unique_ptr<root_t> m_ctrl;
  /**
   * @brief A cache of the top stacklet's `m_lo`
   */
  std::byte *m_lo = nullptr;
  /**
   * @brief The current position of the stack pointer in the stack.
   */
  std::byte *m_sp = nullptr;
  /**
   * @brief A cache of the top stacklet's `m_hi`
   */
  std::byte *m_hi = nullptr;

  /**
   * @brief Capacity of the current top stacklet's stack.
   */
  [[nodiscard]] auto capacity() const noexcept -> std::size_t {
    return detail::checked_cast<std::size_t>(m_hi - m_lo);
  }
  /**
   * @brief Unused space on the top stacklet's stack.
   */
  [[nodiscard]] auto unused() const noexcept -> std::size_t {
    return detail::checked_cast<std::size_t>(m_hi - m_sp);
  }
  /**
   * @brief Check if top stack is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool { return m_sp == m_lo; }
};

} // namespace lf

#endif /* C4F946B7_9F01_49F3_B8F7_B9235E901BD7 */
