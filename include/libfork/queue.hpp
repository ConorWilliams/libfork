#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "libfork/utility.hpp"

/**
 * @file queue.hpp
 *
 * @brief A stand-alone implementation of the Chase-Lev lock-free single-producer multiple-consumer
 * queue.
 *
 * \rst
 *
 * Implements the queue described in the papers, `"Dynamic Circular Work-Stealing Queue"
 * <https://doi.org/10.1145/1073970.1073974>`_ and `"Correct and Efficient Work-Stealing for Weak
 * Memory Models" <https://doi.org/10.1145/2442516.2442524>`_. Both are available in `reference/
 * <https://github.com/ConorWilliams/Forkpool/tree/main/reference>`_.
 *
 * \endrst
 */

namespace lf {

/**
 * @brief A concept for ``std::is_trivial_v<T>``.
 */
template <typename T>
concept Trivial = std::is_trivial_v<T>;

/**
 * @brief A basic wrapper around a c-style array that provides modulo load/stores.
 *
 * This class is designed for internal use only. It provides a c-style API that is used efficiantly
 * by queue for low level atomic operations.
 *
 * @tparam T The type of the elements in the array.
 */
template <Trivial T>
struct ring_buf {
  /**
   * @brief Construct a new ring buff object
   *
   * @param cap The capacity of the buffer, MUST be a power of 2.
   */
  explicit ring_buf(std::int64_t cap) : m_cap{cap}, m_mask{cap - 1} {
    ASSERT(cap && (!(cap & (cap - 1))), "Capacity must be a power of 2!");
  }

  /**
   * @brief Get the capacity of the buffer.
   */
  auto capacity() const noexcept -> std::int64_t { return m_cap; }

  /**
   * @brief Store ``val`` at ``index % this->capacity()``.
   */
  auto store(std::int64_t index, T val) noexcept -> void { m_buf[index & m_mask] = val; }

  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  auto load(std::int64_t index) const noexcept -> T { return m_buf[index & m_mask]; }

  /**
   * @brief Copies elements in range ``[bottom, top)`` into a new ring buffer.
   *
   * This function allocates a new buffer and returns a pointer to it. The caller is responsible for
   * deallocating the memory.
   *
   * @param bottom The bottom of the range to copy from (inclusive).
   * @param top The top of the range to copy from (exclusive).
   */
  auto resize(std::int64_t bottom, std::int64_t top) const -> ring_buf<T>* {
    auto* ptr = new ring_buf{2 * m_cap};
    for (std::int64_t i = top; i != bottom; ++i) {
      ptr->store(i, load(i));
    }
    return ptr;
  }

 private:
  std::int64_t m_cap;   ///< Capacity of the buffer
  std::int64_t m_mask;  ///< Bit mask to perform modulo capacity operations

  std::unique_ptr<T[]> m_buf = std::make_unique_for_overwrite<T[]>(m_cap);
};

/**
 * @brief The cache line size of the current architecture.
 */
#ifdef __cpp_lib_hardware_interference_size
inline constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t k_cache_line = 64;
#endif

/**
 * @brief An unbounded lock-free single-producer multiple-consumer queue.
 *
 * Only the queue owner can perform ``pop()`` and ``push()`` operations where the queue behaves like
 * a LIFO stack. Others can (only) ``steal()`` data from the queue, they see a FIFO queue. All
 * threads must have finished using the queue before it is destructed.
 *
 * @tparam T The type of the elements in the queue - must be a trivial type.
 */
template <Trivial T>
class queue {
  static constexpr std::int64_t k_default_capacity = 1024;
  static constexpr std::size_t k_garbage_reserve = 32;

 public:
  /**
   * @brief Construct a new empty queue object.
   */
  queue() : queue(k_default_capacity) {}

  /**
   * @brief Construct a new empty queue object.
   *
   * @param cap The capacity of the queue (must be a power of 2).
   */
  explicit queue(std::int64_t cap);

  /**
   * @brief Queue's are not copiable or movable.
   */
  queue(queue const& other) = delete;
  /**
   * @brief Queue's are not copiable or movable.
   */
  queue(queue&& other) = delete;
  /**
   * @brief Queue's are not assignable.
   */
  auto operator=(queue const& other) -> queue& = delete;
  /**
   * @brief Queue's are not assignable.
   */
  auto operator=(queue&& other) -> queue& = delete;

  /**
   * @brief Get the number of elements in the queue.
   */
  auto size() const noexcept -> std::size_t;

  /**
   * @brief Get the capacity of the queue.
   */
  auto capacity() const noexcept -> int64_t;

  /**
   * @brief Check if the queue is empty.
   */
  auto empty() const noexcept -> bool;

  /**
   * @brief Push an item into the queue.
   *
   * Only the owner thread can insert an item into the queue. The operation can trigger the queue to
   * resize if more space is required.
   *
   * @param val Value to add to the queue.
   */
  auto push(T const& val) noexcept -> void;

  /**
   * @brief Pop an item from the queue.
   *
   * Only the owner thread can pop out an item from the queue. Returns ``std::nullopt`` if
   * this operation fails (i.e. the queue is empty).
   */
  auto pop() noexcept -> std::optional<T>;

  /**
   * @brief Error codes for the ``steal()`` operation.
   */
  enum class err : int {
    won = 0,  ///< The ``steal()`` operation succeeded.
    lost,     ///< Lost the ``steal()`` race hence, the ``steal()`` operation failed.
    empty,    ///< The queue is empty and hence, the ``steal()`` operation failed.
  };

  /**
   * @brief The return type of the ``steal()`` operation.
   *
   * Suitable for structured bindings.
   */
  struct steal_t {
    /**
     * @brief Check if the operation succeeded.
     */
    constexpr explicit operator bool() const noexcept { return code == err::won; }

    err code;  ///< The error code of the ``steal()`` operation.
    T val;     ///< The value stolen from the queue, Only valid if ``code == err::stolen``.
  };

  /**
   * @brief Steal an item from the queue.
   *
   * Any threads can try to steal an item from the queue. This operation can fail if the queue is
   * empty or if another thread simulatniously stole an item from the queue.
   */
  auto steal() noexcept -> steal_t;

  /**
   * @brief Destroy the queue object.
   *
   * All threads must have finished using the queue before it is destructed.
   */
  ~queue() noexcept;

 private:
  alignas(k_cache_line) std::atomic<std::int64_t> m_top;
  alignas(k_cache_line) std::atomic<std::int64_t> m_bottom;
  alignas(k_cache_line) std::atomic<ring_buf<T>*> m_buf;

  std::vector<std::unique_ptr<ring_buf<T>>> m_garbage;  // Store old buffers here.

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <Trivial T>
queue<T>::queue(std::int64_t cap) : m_top(0), m_bottom(0), m_buf(new ring_buf<T>{cap}) {
  m_garbage.reserve(k_garbage_reserve);
}

template <Trivial T>
auto queue<T>::size() const noexcept -> std::size_t {
  int64_t bottom = m_bottom.load(relaxed);
  int64_t top = m_top.load(relaxed);
  return static_cast<std::size_t>(bottom >= top ? bottom - top : 0);
}

template <Trivial T>
auto queue<T>::capacity() const noexcept -> int64_t {
  return m_buf.load(relaxed)->capacity();
}

template <Trivial T>
auto queue<T>::empty() const noexcept -> bool {
  return !size();
}

template <Trivial T>
auto queue<T>::push(T const& val) noexcept -> void {
  std::int64_t bottom = m_bottom.load(relaxed);
  std::int64_t top = m_top.load(acquire);
  ring_buf<T>* buf = m_buf.load(relaxed);

  if (buf->capacity() < (bottom - top) + 1) {
    // Queue is full, build a new one.
    m_garbage.emplace_back(std::exchange(buf, buf->resize(bottom, top)));
    m_buf.store(buf, relaxed);
  }

  // Construct new object, this does not have to be atomic as no one can steal this item until after
  // we store the new value of bottom, ordering is maintained by surrounding atomics.
  buf->store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);
}

template <Trivial T>
auto queue<T>::pop() noexcept -> std::optional<T> {
  std::int64_t bottom = m_bottom.load(relaxed) - 1;
  ring_buf<T>* buf = m_buf.load(relaxed);

  m_bottom.store(bottom, relaxed);  // Stealers can no longer steal.

  std::atomic_thread_fence(seq_cst);
  std::int64_t top = m_top.load(relaxed);

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

template <Trivial T>
auto queue<T>::steal() noexcept -> steal_t {
  std::int64_t top = m_top.load(acquire);
  std::atomic_thread_fence(seq_cst);
  std::int64_t bottom = m_bottom.load(acquire);

  if (top < bottom) {
    // Must load *before* acquiring the slot as slot may be overwritten immediately after acquiring.
    // This load is NOT required to be atomic even-though it may race with an overwrite as we only
    // return the value if we win the race below garanteeing we had no race during our read. If we
    // loose the race then 'x' could be corrupt due to read-during-write race but as T is trivially
    // destructible this does not matter.
    T tmp = m_buf.load(consume)->load(top);

    if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
      return {.code = err::lost};
    }
    return {.code = err::won, .val = tmp};
  }
  return {.code = err::empty};
}

template <Trivial T>
queue<T>::~queue() noexcept {
  delete m_buf.load();
}

}  // namespace lf