#pragma once

/* This file has been modified by C.J.Williams to act as a standalone
 * version of folly::event_count utilizing c++20's futex wait facilities.
 *
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>
#include <cstdint>
#include <thread>

#include "libfork/utility.hpp"

/**
 * @file event_count.hpp
 *
 * @brief A condition variable for lock free algorithms borrowed from folly.
 */

namespace lf {

namespace detail {

// Endianness
#ifdef _MSC_VER
  // It's MSVC, so we just have to guess ... and allow an override
  #ifdef RIFTEN_ENDIAN_BE
constexpr bool k_is_little_endian = false;
  #else
constexpr bool k_is_little_endian = true;
  #endif
#else
constexpr bool k_is_little_endian = __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#endif

}  // namespace detail

/**
 * @brief A condition variable for lock free algorithms.
 *
 * \rst
 *
 * See http://www.1024cores.net/home/lock-free-algorithms/eventcounts for details.
 *
 * Event counts allow you to convert a non-blocking lock-free / wait-free
 * algorithm into a blocking one, by isolating the blocking logic.  You call
 * prepare_wait() before checking your condition and then either cancel_wait()
 * or wait() depending on whether the condition was true.  When another
 * thread makes the condition true, it must call notify() / notify_all() just
 * like a regular condition variable.
 *
 * If "<" denotes the happens-before relationship, consider 2 threads (T1 and
 * T2) and 3 events:
 *
 * * E1: T1 returns from prepare_wait.
 * * E2: T1 calls wait (obviously E1 < E2, intra-thread).
 * * E3: T2 calls ``notify_all()``.
 *
 * If E1 < E3, then E2's wait will complete (and T1 will either wake up,
 * or not block at all)
 *
 * This means that you can use an event_count in the following manner:
 *
 * Waiter:
 *
 * .. code::
 *
 *    if (!condition()) {  // handle fast path first
 *      for (;;) {
 *        auto key = eventCount.prepare_wait();
 *        if (condition()) {
 *          eventCount.cancel_wait();
 *          break;
 *        } else {
 *          eventCount.wait(key);
 *        }
 *      }
 *    }
 *
 * Poster:
 *
 * .. code::
 *
 *    make_condition_true();
 *    eventCount.notify_all();
 *
 * .. note::
 *
 *    Just like with regular condition variables, the waiter needs to
 *    be tolerant of spurious wakeups and needs to recheck the condition after
 *    being woken up.  Also, as there is no mutual exclusion implied, "checking"
 *    the condition likely means attempting an operation on an underlying
 *    data structure (push into a lock-free queue, etc) and returning true on
 *    success and false on failure.
 *
 * \endrst
 */
class event_count {
 public:
  event_count() = default;
  event_count(event_count const&) = delete;
  event_count(event_count&&) = delete;
  auto operator=(event_count const&) -> event_count& = delete;
  auto operator=(event_count&&) -> event_count& = delete;
  ~event_count() = default;

  /**
   * @brief The return type of ``prepare_wait()``.
   */
  class key {
    friend class event_count;
    explicit key(uint32_t epoch) noexcept : m_epoch(epoch) {}
    std::uint32_t m_epoch;
  };
  /**
   * @brief Wake up one waiter.
   */
  auto notify_one() noexcept -> void;
  /**
   * @brief Wake up all waiters.
   */
  auto notify_all() noexcept -> void;
  /**
   * @brief Prepare to wait.
   *
   * Once this has been called, you must either call ``wait()`` with the key or ``cancel_wait()``.
   */
  [[nodiscard]] auto prepare_wait() noexcept -> key;
  /**
   * @brief Cancel a wait.
   */
  auto cancel_wait() noexcept -> void;
  /**
   * @brief Wait for a notification, this blocks the current thread.
   */
  auto wait(key key) noexcept -> void;

 private:
  auto epoch() noexcept -> std::atomic<std::uint32_t>* {
    // NOLINTNEXTLINE
    return reinterpret_cast<std::atomic<std::uint32_t>*>(&m_val) + k_epoch_offset;
  }

  // This requires 64-bit
  static_assert(sizeof(int) == 4, "bad platform, need 64bits");
  static_assert(sizeof(std::uint32_t) == 4, "bad platform, need 64bits");
  static_assert(sizeof(std::uint64_t) == 8, "bad platform, need 64bits");
  static_assert(sizeof(std::atomic<std::uint32_t>) == 4, "bad platform, need 64bits");
  static_assert(sizeof(std::atomic<std::uint64_t>) == 8, "bad platform, need 64bits");

  static constexpr size_t k_epoch_offset = detail::k_is_little_endian ? 1 : 0;

  static constexpr std::uint64_t k_add_waiter = 1;
  static constexpr std::uint64_t k_sub_waiter = -1;
  static constexpr std::uint64_t k_epoch_shift = 32;
  static constexpr std::uint64_t k_add_epoch = static_cast<std::uint64_t>(1) << k_epoch_shift;
  static constexpr std::uint64_t k_waiter_mask = k_add_epoch - 1;

  // m_val stores the epoch in the most significant 32 bits and the
  // waiter count in the least significant 32 bits.
  alignas(detail::k_cache_line) std::atomic<std::uint64_t> m_val = 0;
};

inline void event_count::notify_one() noexcept {
  if (m_val.fetch_add(k_add_epoch, std::memory_order_acq_rel) & k_waiter_mask) {  // NOLINT
    epoch()->notify_one();
  }
}

inline void event_count::notify_all() noexcept {
  if (m_val.fetch_add(k_add_epoch, std::memory_order_acq_rel) & k_waiter_mask) {  // NOLINT
    epoch()->notify_all();
  }
}

[[nodiscard]] inline auto event_count::prepare_wait() noexcept -> event_count::key {
  std::uint64_t prev = m_val.fetch_add(k_add_waiter, std::memory_order_acq_rel);
  return key(prev >> k_epoch_shift);
}

inline void event_count::cancel_wait() noexcept {
  // memory_order_relaxed would suffice for correctness, but the faster
  // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
  // (and thus system calls).
  m_val.fetch_add(k_sub_waiter, std::memory_order_seq_cst);
}

inline void event_count::wait(key key) noexcept {
  // Use C++20 atomic wait guarantees
  epoch()->wait(key.m_epoch, std::memory_order_acquire);

  // memory_order_relaxed would suffice for correctness, but the faster
  // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
  // (and thus system calls)
  m_val.fetch_add(k_sub_waiter, std::memory_order_seq_cst);
}

}  // namespace lf