/* This file has been modified by C.J.Williams to act as a standalone
 * version of folly::EventCount utilising c++20's futex wait facilities.
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

#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

namespace riften {

namespace detail {

// Endianness
#ifdef _MSC_VER
// It's MSVC, so we just have to guess ... and allow an override
#    ifdef RIFTEN_ENDIAN_BE
constexpr auto kIsLittleEndian = false;
#    else
constexpr auto kIsLittleEndian = true;
#    endif
#else
constexpr auto kIsLittleEndian = __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#endif

}  // namespace detail

/**
 * Event count: a condition variable for lock free algorithms.
 *
 * See http://www.1024cores.net/home/lock-free-algorithms/eventcounts for
 * details.
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
 * - E1: T1 returns from prepare_wait
 * - E2: T1 calls wait
 *   (obviously E1 < E2, intra-thread)
 * - E3: T2 calls notify_all
 *
 * If E1 < E3, then E2's wait will complete (and T1 will either wake up,
 * or not block at all)
 *
 * This means that you can use an EventCount in the following manner:
 *
 * Waiter:
 *   if (!condition()) {  // handle fast path first
 *     for (;;) {
 *       auto key = eventCount.prepare_wait();
 *       if (condition()) {
 *         eventCount.cancel_wait();
 *         break;
 *       } else {
 *         eventCount.wait(key);
 *       }
 *     }
 *  }
 *
 *
 * Poster:
 *   make_condition_true();
 *   eventCount.notify_all();
 *
 * Note that, just like with regular condition variables, the waiter needs to
 * be tolerant of spurious wakeups and needs to recheck the condition after
 * being woken up.  Also, as there is no mutual exclusion implied, "checking"
 * the condition likely means attempting an operation on an underlying
 * data structure (push into a lock-free queue, etc) and returning true on
 * success and false on failure.
 */
class EventCount {
  public:
    EventCount() noexcept : _val(0) {}

    class Key {
        friend class EventCount;
        explicit Key(uint32_t e) noexcept : epoch_(e) {}
        uint32_t epoch_;
    };

    void notify_one() noexcept;
    void notify_all() noexcept;

    [[nodiscard]] Key prepare_wait() noexcept;

    void cancel_wait() noexcept;
    void wait(Key key) noexcept;

  private:
    std::atomic<uint32_t>* epoch() noexcept {
        return reinterpret_cast<std::atomic<uint32_t>*>(&_val) + kEpochOffset;
    }

    EventCount(const EventCount&) = delete;
    EventCount(EventCount&&) = delete;
    EventCount& operator=(const EventCount&) = delete;
    EventCount& operator=(EventCount&&) = delete;

    // This requires 64-bit
    static_assert(sizeof(int) == 4, "bad platform");
    static_assert(sizeof(uint32_t) == 4, "bad platform");
    static_assert(sizeof(uint64_t) == 8, "bad platform");
    static_assert(sizeof(std::atomic<uint32_t>) == 4, "bad platform");
    static_assert(sizeof(std::atomic<uint64_t>) == 8, "bad platform");

    static constexpr size_t kEpochOffset = detail::kIsLittleEndian ? 1 : 0;

    // _val stores the epoch in the most significant 32 bits and the
    // waiter count in the least significant 32 bits.
    alignas(hardware_destructive_interference_size) std::atomic<uint64_t> _val;

    static constexpr uint64_t kAddWaiter = uint64_t(1);
    static constexpr uint64_t kSubWaiter = uint64_t(-1);
    static constexpr uint64_t kEpochShift = 32;
    static constexpr uint64_t kAddEpoch = uint64_t(1) << kEpochShift;
    static constexpr uint64_t kWaiterMask = kAddEpoch - 1;
};

inline void EventCount::notify_one() noexcept {
    if (_val.fetch_add(kAddEpoch, std::memory_order_acq_rel) & kWaiterMask) {
        epoch()->notify_one();
    }
}

inline void EventCount::notify_all() noexcept {
    if (_val.fetch_add(kAddEpoch, std::memory_order_acq_rel) & kWaiterMask) {
        epoch()->notify_all();
    }
}

[[nodiscard]] inline EventCount::Key EventCount::prepare_wait() noexcept {
    uint64_t prev = _val.fetch_add(kAddWaiter, std::memory_order_acq_rel);
    return Key(prev >> kEpochShift);
}

inline void EventCount::cancel_wait() noexcept {
    // memory_order_relaxed would suffice for correctness, but the faster
    // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
    // (and thus system calls).
    _val.fetch_add(kSubWaiter, std::memory_order_seq_cst);
}

inline void EventCount::wait(Key key) noexcept {
    // Use C++20 atomic wait garantees
    epoch()->wait(key.epoch_, std::memory_order_acquire);

    // memory_order_relaxed would suffice for correctness, but the faster
    // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
    // (and thus system calls)
    _val.fetch_add(kSubWaiter, std::memory_order_seq_cst);
}

}  // namespace riften