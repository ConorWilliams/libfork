#ifndef DD6F6C5C_C146_4C02_99B9_7D2D132C0844
#define DD6F6C5C_C146_4C02_99B9_7D2D132C0844

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <coroutine>
#include <semaphore>
#include <type_traits>

#include "libfork/core/impl/stack.hpp"

/**
 * @file frame.hpp
 *
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */

namespace lf::impl {

/**
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */
class frame {

#ifndef LF_COROUTINE_OFFSET
  std::coroutine_handle<> m_this_coro; ///< Handle to this coroutine.
#endif

  stack::stacklet *m_stacklet; ///< Needs to be in promise in case allocation elided (as does m_parent).
  union {
    frame *m_parent;              ///< Non-root tasks store a pointer to their parent.
    std::binary_semaphore *m_sem; ///< Root tasks store a pointer to a semaphore.
  };
  std::atomic_uint16_t m_join = k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;               ///< Number of times this frame has been stolen.

 public:
  /**
   * @brief Construct a frame block.
   *
   * Non-root tasks will need to call ``set_parent(...)``.
   */
#ifndef LF_COROUTINE_OFFSET
  frame(std::coroutine_handle<> coro, stack::stacklet *stacklet) noexcept
      : m_this_coro{coro},
        m_stacklet(non_null(stacklet)) {
    LF_ASSERT(coro);
  }
#else
  frame(std::coroutine_handle<>, stack::stacklet *stacklet) noexcept : m_stacklet(non_null(stacklet)) {}
#endif

  /**
   * @brief Set the pointer to the parent frame.
   */
  void set_parent(frame *parent) noexcept { m_parent = non_null(parent); }

  /**
   * @brief Set the pointer to the semaphore.
   */
  void set_semaphore(std::binary_semaphore *sem) noexcept { m_sem = non_null(sem); }

  /**
   * @brief Set the stacklet object.
   */
  void set_stacklet(stack::stacklet *stacklet) noexcept { m_stacklet = non_null(stacklet); }

  /**
   * @brief Get a pointer to the parent frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame * { return m_parent; }

  /**
   * @brief Get a pointer to the parent frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto semaphore() const noexcept -> std::binary_semaphore * { return m_sem; }

  /**
   * @brief Get a pointer to the top of the top of the stack-stack this frame was allocated on.
   */
  [[nodiscard]] auto stacklet() const noexcept -> stack::stacklet * { return non_null(m_stacklet); }

  /**
   * @brief Get the coroutine handle for this frames coroutine.
   */
  [[nodiscard]] auto self() noexcept -> std::coroutine_handle<> {
#ifndef LF_COROUTINE_OFFSET
    return m_this_coro;
#else
    return std::coroutine_handle<>::from_address(byte_cast(this) - LF_COROUTINE_OFFSET);
#endif
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] auto load_joins(std::memory_order order) const noexcept -> std::uint16_t {
    return m_join.load(order);
  }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  auto fetch_sub_joins(std::uint16_t val, std::memory_order order) noexcept -> std::uint16_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] auto load_steals() const noexcept -> std::uint16_t { return m_steal; }

  /**
   * @brief Increase the steal counter by one and return the previous value.
   */
  auto fetch_add_steal() noexcept -> std::uint16_t { return m_steal++; }

  /**
   * @brief Reset the join and steal counters, must be outside a fork-join region.
   */
  void reset() noexcept {

    m_steal = 0;

    static_assert(std::is_trivially_destructible_v<decltype(m_join)>);
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    std::construct_at(&m_join, k_u16_max);
  }
};

static_assert(std::is_standard_layout_v<frame>);

} // namespace lf::impl

#endif /* DD6F6C5C_C146_4C02_99B9_7D2D132C0844 */
