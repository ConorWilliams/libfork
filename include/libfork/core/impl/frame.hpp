#ifndef DD6F6C5C_C146_4C02_99B9_7D2D132C0844
#define DD6F6C5C_C146_4C02_99B9_7D2D132C0844

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>      // for atomic_ref, memory_order, atomic_uint16_t
#include <coroutine>   // for coroutine_handle
#include <cstdint>     // for uint16_t
#include <exception>   // for exception_ptr, operator==, current_exce...
#include <memory>      // for construct_at
#include <semaphore>   // for binary_semaphore
#include <type_traits> // for is_standard_layout_v, is_trivially_dest...
#include <utility>     // for exchange
#include <version>     // for __cpp_lib_atomic_ref

#include "libfork/core/defer.hpp"                // for LF_DEFER
#include "libfork/core/impl/manual_lifetime.hpp" // for manual_lifetime
#include "libfork/core/impl/stack.hpp"           // for stack
#include "libfork/core/impl/utility.hpp"         // for non_null, k_u16_max
#include "libfork/core/macro.hpp"                // for LF_COMPILER_EXCEPTIONS, LF_ASSERT, LF_F...

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

#if LF_COMPILER_EXCEPTIONS
  /**
   * @brief Maybe an exception pointer.
   */
  manual_lifetime<std::exception_ptr> m_eptr;
#endif

#ifndef LF_COROUTINE_OFFSET
  /**
   * @brief Handle to this coroutine, inferred from `this` if `LF_COROUTINE_OFFSET` is set.
   */
  std::coroutine_handle<> m_this_coro;
#endif
  /**
   * @brief This frames stacklet, needs to be in promise in case allocation elided (as does m_parent).
   */
  stack::stacklet *m_stacklet;

  union {
    /**
     * @brief Non-root tasks store a pointer to their parent.
     */
    frame *m_parent;
    /**
     * @brief Root tasks store a pointer to a semaphore to notify the caller.
     */
    std::binary_semaphore *m_sem;
  };

  /**
   * @brief  Number of children joined (with offset).
   */
  std::atomic_uint16_t m_join = k_u16_max;
  /**
   * @brief Number of times this frame has been stolen.
   */
  std::uint16_t m_steal = 0;

/**
 * @brief Flag to indicate if an exception has been set.
 */
#if LF_COMPILER_EXCEPTIONS
  #ifdef __cpp_lib_atomic_ref
  bool m_except = false;
  #else
  std::atomic_bool m_except = false;
  #endif
#endif

  /**
   * @brief Cold path in `rethrow_if_exception` in its own non-inline function.
   */
  LF_NOINLINE void rethrow() {
#if LF_COMPILER_EXCEPTIONS

    LF_ASSERT(*m_eptr != nullptr);

    LF_DEFER {
      LF_ASSERT(*m_eptr == nullptr);
      m_eptr.destroy();
  #ifdef __cpp_lib_atomic_ref
      m_except = false;
  #else
      m_except.store(false, std::memory_order_relaxed);
  #endif
    };

    std::rethrow_exception(std::exchange(*m_eptr, nullptr));
#endif
  }

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
   * @brief Set a root tasks parent.
   */
  void set_root_sem(std::binary_semaphore *sem) noexcept { m_sem = non_null(sem); }

  /**
   * @brief Set the stacklet object to point at a new stacklet.
   *
   * Returns the previous stacklet.
   */
  auto reset_stacklet(stack::stacklet *stacklet) noexcept -> stack::stacklet * {
    return std::exchange(m_stacklet, non_null(stacklet));
  }

  /**
   * @brief Get a pointer to the parent frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame * { return m_parent; }

  /**
   * @brief Get a pointer to the semaphore for this root frame.
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

  /**
   * @brief Capture the exception currently being thrown.
   *
   * Safe to call concurrently, first exception is saved.
   */
  void capture_exception() noexcept {
#if LF_COMPILER_EXCEPTIONS
  #ifdef __cpp_lib_atomic_ref
    bool prev = std::atomic_ref{m_except}.exchange(true, std::memory_order_acq_rel);
  #else
    bool prev = m_except.exchange(true, std::memory_order_acq_rel);
  #endif

    if (!prev) {
      m_eptr.construct(std::current_exception());
    }
#endif
  }

  /**
   * @brief Test if the exception flag is set.
   *
   * Safe to call concurrently.
   */
  auto atomic_has_exception() const noexcept -> bool {
#if LF_COMPILER_EXCEPTIONS
  #ifdef __cpp_lib_atomic_ref
    return std::atomic_ref{m_except}.load(std::memory_order_acquire);
  #else
    return m_except.load(std::memory_order_acquire);
  #endif
#else
    return false;
#endif
  }

  /**
   * @brief If this contains an exception then it will be rethrown and this this object reset to the OK state.
   *
   * This can __only__ be called when the caller has exclusive ownership over this object.
   */
  LF_FORCEINLINE void rethrow_if_exception() {
#if LF_COMPILER_EXCEPTIONS
  #ifdef __cpp_lib_atomic_ref
    if (m_except) {
  #else
    if (m_except.load(std::memory_order_relaxed)) {
  #endif
      rethrow();
    }
#endif
  }

  /**
   * @brief Check if this contains an exception.
   *
   * This can __only__ be called when the caller has exclusive ownership over this object.
   */
  [[nodiscard]] auto has_exception() const noexcept -> bool {
#if LF_COMPILER_EXCEPTIONS
  #ifdef __cpp_lib_atomic_ref
    return m_except;
  #else
    return m_except.load(std::memory_order_relaxed);
  #endif
#else
    return false;
#endif
  }
};

static_assert(std::is_standard_layout_v<frame>);

} // namespace lf::impl

#endif /* DD6F6C5C_C146_4C02_99B9_7D2D132C0844 */
