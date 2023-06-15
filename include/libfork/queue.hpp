#ifndef C9703881_3D9C_41A5_A7A2_44615C4CFA6A
#define C9703881_3D9C_41A5_A7A2_44615C4CFA6A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "libfork/macro.hpp" // Only for ASSERT macro + hardware_destructive_interference_size

/**
 * @file queue.hpp
 *
 * @brief A stand-alone, production-quality implementation of the Chase-Lev lock-free
 * single-producer multiple-consumer queue.
 *
 * \rst
 *
 * Implements the queue described in the papers, `"Dynamic Circular Work-Stealing Queue"
 * <https://doi.org/10.1145/1073970.1073974>`_ and `"Correct and Efficient Work-Stealing for Weak
 * Memory Models" <https://doi.org/10.1145/2442516.2442524>`_. Both are available in `reference/
 * <https://github.com/ConorWilliams/libfork/tree/main/reference>`_.
 *
 * \endrst
 */

namespace lf {

/**
 * @brief A concept that verifies a type is suitable for use in a queue.
 */
template <typename T>
concept simple = std::is_default_constructible_v<T> && std::is_trivially_copyable_v<T> && std::atomic<T>::is_always_lock_free;

namespace detail {

/**
 * @brief A basic wrapper around a c-style array that provides modulo load/stores.
 *
 * This class is designed for internal use only. It provides a c-style API that is
 * used efficiently by queue for low level atomic operations.
 *
 * @tparam T The type of the elements in the array.
 */
template <simple T>
struct ring_buf {
  /**
   * @brief Construct a new ring buff object
   *
   * @param cap The capacity of the buffer, MUST be a power of 2.
   */
  explicit ring_buf(std::ptrdiff_t cap) : m_cap{cap}, m_mask{cap - 1} {
    LIBFORK_ASSERT(cap > 0 && std::has_single_bit(static_cast<std::size_t>(cap)));
  }
  /**
   * @brief Get the capacity of the buffer.
   */
  [[nodiscard]] auto capacity() const noexcept -> std::ptrdiff_t { return m_cap; }
  /**
   * @brief Store ``val`` at ``index % this->capacity()``.
   */
  auto store(std::ptrdiff_t index, T val) noexcept -> void {
    LIBFORK_ASSERT(index >= 0);
    (m_buf.get() + (index & m_mask))->store(val, std::memory_order_relaxed); // NOLINT Avoid cast to std::size_t.
  }
  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  [[nodiscard]] auto load(std::ptrdiff_t index) const noexcept -> T {
    LIBFORK_ASSERT(index >= 0);
    return (m_buf.get() + (index & m_mask))->load(std::memory_order_relaxed); // NOLINT Avoid cast to std::size_t.
  }
  /**
   * @brief Copies elements in range ``[bottom, top)`` into a new ring buffer.
   *
   * This function allocates a new buffer and returns a pointer to it. The caller is responsible
   * for deallocating the memory.
   *
   * @param bottom The bottom of the range to copy from (inclusive).
   * @param top The top of the range to copy from (exclusive).
   */
  auto resize(std::ptrdiff_t bottom, std::ptrdiff_t top) const -> ring_buf<T> * { // NOLINT
    auto *ptr = new ring_buf{2 * m_cap};                                          // NOLINT
    for (std::ptrdiff_t i = top; i != bottom; ++i) {
      ptr->store(i, load(i));
    }
    return ptr;
  }

private:
  using array_t = std::atomic<T>[]; // NOLINT

  std::ptrdiff_t m_cap;  ///< Capacity of the buffer
  std::ptrdiff_t m_mask; ///< Bit mask to perform modulo capacity operations

#ifdef __cpp_lib_smart_ptr_for_overwrite
  std::unique_ptr<array_t> m_buf = std::make_unique_for_overwrite<array_t>(static_cast<std::size_t>(m_cap));
#else
  std::unique_ptr<array_t> m_buf = std::make_unique<array_t>(static_cast<std::size_t>(m_cap));
#endif
};

} // namespace detail

/**
 * @brief Error codes for ``queue`` 's ``steal()`` operation.
 */
enum class err : int {
  none = 0, ///< The ``steal()`` operation succeeded.
  lost,     ///< Lost the ``steal()`` race hence, the ``steal()`` operation failed.
  empty,    ///< The queue is empty and hence, the ``steal()`` operation failed.
};

/**
 * @brief An unbounded lock-free single-producer multiple-consumer queue.
 *
 * Only the queue owner can perform ``pop()`` and ``push()`` operations where the queue behaves
 * like a LIFO stack. Others can (only) ``steal()`` data from the queue, they see a FIFO queue.
 * All threads must have finished using the queue before it is destructed.
 *
 * \rst
 *
 * Example:
 *
 * .. include:: ../../test/glob/queue.cpp
 *    :code:
 *    :start-after: // !BEGIN-EXAMPLE
 *    :end-before: // !END-EXAMPLE
 *
 * \endrst
 *
 * @tparam T The type of the elements in the queue - must be a simple type.
 */
template <simple T>
class queue {
  static constexpr std::ptrdiff_t k_default_capacity = 1024;
  static constexpr std::size_t k_garbage_reserve = 32;

public:
  /**
   * @brief The type of the elements in the queue.
   */
  using value_type = T;
  /**
   * @brief Construct a new empty queue object.
   */
  queue() : queue(k_default_capacity) {}
  /**
   * @brief Construct a new empty queue object.
   *
   * @param cap The capacity of the queue (must be a power of 2).
   */
  explicit queue(std::ptrdiff_t cap);
  /**
   * @brief Queue's are not copiable or movable.
   */
  queue(queue const &other) = delete;
  /**
   * @brief Queue's are not copiable or movable.
   */
  queue(queue &&other) = delete;
  /**
   * @brief Queue's are not assignable.
   */
  auto operator=(queue const &other) -> queue & = delete;
  /**
   * @brief Queue's are not assignable.
   */
  auto operator=(queue &&other) -> queue & = delete;
  /**
   * @brief Get the number of elements in the queue.
   */
  [[nodiscard]] auto size() const noexcept -> std::size_t;
  /**
   * @brief Get the number of elements in the queue as a signed integer.
   */
  [[nodiscard]] auto ssize() const noexcept -> ptrdiff_t;
  /**
   * @brief Get the capacity of the queue.
   */
  [[nodiscard]] auto capacity() const noexcept -> ptrdiff_t;
  /**
   * @brief Check if the queue is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool;
  /**
   * @brief Push an item into the queue.
   *
   * Only the owner thread can insert an item into the queue. The operation can trigger the queue
   * to resize if more space is required.
   *
   * @param val Value to add to the queue.
   */
  auto push(T const &val) noexcept -> void;
  /**
   * @brief Pop an item from the queue.
   *
   * Only the owner thread can pop out an item from the queue. Returns ``std::nullopt`` if
   * this operation fails (i.e. the queue is empty).
   */
  auto pop() noexcept -> std::optional<T>;
  /**
   * @brief The return type of the ``steal()`` operation.
   *
   * Suitable for structured bindings.
   */
  struct steal_t {
    /**
     * @brief Check if the operation succeeded.
     */
    constexpr explicit operator bool() const noexcept { return code == err::none; }
    /**
     * @brief Get the value like ``std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator*() noexcept -> T {
      LIBFORK_ASSERT(code == err::none);
      return val;
    }
    /**
     * @brief Get the value ``like std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator->() noexcept -> T * {
      LIBFORK_ASSERT(code == err::none);
      return std::addressof(val);
    }
    /**
     * @brief Get the value ``like std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator->() const noexcept -> T const * {
      LIBFORK_ASSERT(code == err::none);
      return std::addressof(val);
    }

    err code; ///< The error code of the ``steal()`` operation.
    T val;    ///< The value stolen from the queue, Only valid if ``code == err::stolen``.
  };

  /**
   * @brief Steal an item from the queue.
   *
   * Any threads can try to steal an item from the queue. This operation can fail if the queue is
   * empty or if another thread simultaneously stole an item from the queue.
   */
  auto steal() noexcept -> steal_t;
  /**
   * @brief Destroy the queue object.
   *
   * All threads must have finished using the queue before it is destructed.
   */
  ~queue() noexcept;

private:
  alignas(detail::k_cache_line) std::atomic<std::ptrdiff_t> m_top;
  alignas(detail::k_cache_line) std::atomic<std::ptrdiff_t> m_bottom;
  alignas(detail::k_cache_line) std::atomic<detail::ring_buf<T> *> m_buf;

  alignas(detail::k_cache_line) std::vector<std::unique_ptr<detail::ring_buf<T>>> m_garbage; // Store old buffers here.

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <simple T>
queue<T>::queue(std::ptrdiff_t cap) : m_top(0), m_bottom(0), m_buf(new detail::ring_buf<T>{cap}) {
  m_garbage.reserve(k_garbage_reserve);
}

template <simple T>
auto queue<T>::size() const noexcept -> std::size_t {
  return static_cast<std::size_t>(ssize());
}

template <simple T>
auto queue<T>::ssize() const noexcept -> std::ptrdiff_t {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return std::max(bottom - top, ptrdiff_t{0});
}

template <simple T>
auto queue<T>::capacity() const noexcept -> ptrdiff_t {
  return m_buf.load(relaxed)->capacity();
}

template <simple T>
auto queue<T>::empty() const noexcept -> bool {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return top >= bottom;
}

template <simple T>
auto queue<T>::push(T const &val) noexcept -> void {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(acquire);
  detail::ring_buf<T> *buf = m_buf.load(relaxed);

  if (buf->capacity() < (bottom - top) + 1) {
    // Queue is full, build a new one.
    m_garbage.emplace_back(std::exchange(buf, buf->resize(bottom, top)));
    m_buf.store(buf, relaxed);
  }

  // Construct new object, this does not have to be atomic as no one can steal this item until
  // after we store the new value of bottom, ordering is maintained by surrounding atomics.
  buf->store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);
}

template <simple T>
auto queue<T>::pop() noexcept -> std::optional<T> {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed) - 1;
  detail::ring_buf<T> *buf = m_buf.load(relaxed);

  m_bottom.store(bottom, relaxed); // Stealers can no longer steal.

  std::atomic_thread_fence(seq_cst);
  std::ptrdiff_t top = m_top.load(relaxed);

  if (top <= bottom) {
    // Non-empty queue
    if (top == bottom) {
      // The last item could get stolen, by a stealer that loaded bottom before our write above.
      if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
        // Failed race, thief got the last item.
        m_bottom.store(bottom + 1, relaxed);
        return std::nullopt;
      }
      m_bottom.store(bottom + 1, relaxed);
    }
    // Can delay load until after acquiring slot as only this thread can push(), this load is not
    // required to be atomic as we are the exclusive writer.
    return buf->load(bottom);
  }
  m_bottom.store(bottom + 1, relaxed);
  return std::nullopt;
}

template <simple T>
auto queue<T>::steal() noexcept -> steal_t {
  std::ptrdiff_t top = m_top.load(acquire);
  std::atomic_thread_fence(seq_cst);
  std::ptrdiff_t const bottom = m_bottom.load(acquire);

  if (top < bottom) {
    // Must load *before* acquiring the slot as slot may be overwritten immediately after
    // acquiring. This load is NOT required to be atomic even-though it may race with an overwrite
    // as we only return the value if we win the race below guaranteeing we had no race during our
    // read. If we loose the race then 'x' could be corrupt due to read-during-write race but as T
    // is trivially destructible this does not matter.
    T tmp = m_buf.load(consume)->load(top);

    if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
      return {.code = err::lost, .val = {}};
    }
    return {.code = err::none, .val = tmp};
  }
  return {.code = err::empty, .val = {}};
}

template <simple T>
queue<T>::~queue() noexcept {
  delete m_buf.load(); // NOLINT
}

} // namespace lf

#endif /* C9703881_3D9C_41A5_A7A2_44615C4CFA6A */
