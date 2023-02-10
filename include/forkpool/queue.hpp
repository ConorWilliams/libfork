// #pragma once

// // Copyright © Conor Williams <conorwilliams@outlook.com>

// // SPDX-License-Identifier: MPL-2.0

// // This Source Code Form is subject to the terms of the Mozilla Public
// // License, v. 2.0. If a copy of the MPL was not distributed with this
// // file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #include <atomic>
// #include <cassert>
// #include <concepts>
// #include <cstddef>
// #include <cstdint>
// #include <memory>
// #include <new>
// #include <optional>
// #include <type_traits>
// #include <utility>
// #include <vector>

#include "forkpool/utility.hpp"

// /**
//  * @file queue.hpp
//  *
//  * @brief A stand-alone implementation of lock-free single-producer multiple-consumer dequeue.
//  *
//  * Implements the dequeue described in the papers, "Correct and Efficient Work-Stealing for Weak
//  * Memory Models," and "Dynamic Circular Work-Stealing Dequeue". Both are available in
//  'reference/'.
//  */

// namespace fp {

// template <typename T>
// concept Trivial = std::is_trivial_v<T>;

// namespace detail {

// // Basic wrapper around a c-style array of atomic objects that provides modulo
// // load/stores. Capacity must be a power of 2.
// template <Trivial T>
// struct ring_buf {
//   /**
//    * @brief Construct a new ring buff object
//    *
//    * @param cap
//    */
//   explicit ring_buf(std::int64_t cap) : _cap{cap}, _mask{cap - 1} {
//     assert(cap && (!(cap & (cap - 1))) && "Capacity must be buf power of 2!");
//   }

//   auto capacity() const noexcept -> std::int64_t { return _cap; }

//   // Store (copy) at modulo index
//   auto store(std::int64_t i, T&& x) noexcept -> void
//     requires std::is_nothrow_move_assignable_v<T>
//   {
//     _buff[i & _mask] = std::move(x);
//   }

//   // Load (copy) at modulo index
//   auto load(std::int64_t i) const noexcept -> T
//     requires std::is_nothrow_move_constructible_v<T>
//   {
//     return _buff[i & _mask];
//   }

//   // Allocates and returns a new ring buffer, copies elements in range [b, t)
//   // into the new buffer.
//   ring_buf<T>* resize(std::int64_t b, std::int64_t t) const {
//     ring_buf<T>* ptr = new ring_buf{2 * _cap};
//     for (std::int64_t i = t; i != b; ++i) {
//       ptr->store(i, load(i));
//     }
//     return ptr;
//   }

//  private:
//   std::int64_t _cap;   // Capacity of the buffer
//   std::int64_t _mask;  // Bit mask to perform modulo capacity operations

//   std::unique_ptr<T[]> _buff = std::make_unique_for_overwrite<T[]>(_cap);
// };

// }  // namespace detail

// #ifdef __cpp_lib_hardware_interference_size
// using std::hardware_destructive_interference_size;
// #else
// // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │
// // ...
// inline constexpr std::size_t hardware_destructive_interference_size = 2 *
// sizeof(std::max_align_t); #endif

// // Lock-free single-producer multiple-consumer dequeue. Only the dequeue owner can
// // perform pop and push operations where the dequeue behaves like a stack. Others
// // can (only) steal data from the dequeue, they see a FIFO queue. All threads must
// // have finished using the dequeue before it is destructed. T must be default
// // initializable, trivially destructible and have nothrow move
// // constructor/assignment operators.
// template <Trivial T>
// class Dequeue {
//  public:
//   // Constructs the dequeue with a given capacity the capacity of the dequeue (must
//   // be power of 2)
//   explicit Dequeue(std::int64_t cap = 1024);

//   // Move/Copy is not supported
//   Dequeue(Dequeue const& other) = delete;
//   Dequeue& operator=(Dequeue const& other) = delete;

//   //  Query the size at instance of call
//   std::size_t size() const noexcept;

//   // Query the capacity at instance of call
//   int64_t capacity() const noexcept;

//   // Test if empty at instance of call
//   bool empty() const noexcept;

//   // Emplace an item to the dequeue. Only the owner thread can insert an item to
//   // the dequeue. The operation can trigger the dequeue to resize its cap if more
//   // space is required. Provides the strong exception guarantee.
//   template <typename... Args>
//   void emplace(Args&&... args);

//   // Pops out an item from the dequeue. Only the owner thread can pop out an item
//   // from the dequeue. The return can be a std::nullopt if this operation fails
//   // (empty dequeue).
//   std::optional<T> pop() noexcept;

//   // Steals an item from the dequeue Any threads can try to steal an item from the
//   // dequeue. The return can be a std::nullopt if this operation failed (not
//   // necessarily empty).
//   std::optional<T> steal() noexcept;

//   // Destruct the dequeue, all threads must have finished using the dequeue.
//   ~Dequeue() noexcept;

//  private:
//   alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _top;
//   alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _bottom;
//   alignas(hardware_destructive_interference_size) std::atomic<detail::ring_buf<T>*> _buffer;

//   std::vector<std::unique_ptr<detail::ring_buf<T>>> _garbage;  // Store old buffers here.

//   // Convenience aliases.
//   static constexpr std::memory_order relaxed = std::memory_order_relaxed;
//   static constexpr std::memory_order consume = std::memory_order_consume;
//   static constexpr std::memory_order acquire = std::memory_order_acquire;
//   static constexpr std::memory_order release = std::memory_order_release;
//   static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
// };

// template <Trivial T>
// Dequeue<T>::Dequeue(std::int64_t cap) : _top(0), _bottom(0), _buffer(new
// detail::ring_buf<T>{cap}) {
//   _garbage.reserve(32);
// }

// template <Trivial T>
// std::size_t Dequeue<T>::size() const noexcept {
//   int64_t b = _bottom.load(relaxed);
//   int64_t t = _top.load(relaxed);
//   return static_cast<std::size_t>(b >= t ? b - t : 0);
// }

// template <Trivial T>
// int64_t Dequeue<T>::capacity() const noexcept {
//   return _buffer.load(relaxed)->capacity();
// }

// template <Trivial T>
// bool Dequeue<T>::empty() const noexcept {
//   return !size();
// }

// template <Trivial T>
// template <typename... Args>
// void Dequeue<T>::emplace(Args&&... args) {
//   // Construct before acquiring slot in-case constructor throws
//   T object(std::forward<Args>(args)...);

//   std::int64_t b = _bottom.load(relaxed);
//   std::int64_t t = _top.load(acquire);
//   detail::ring_buf<T>* buf = _buffer.load(relaxed);

//   if (buf->capacity() < (b - t) + 1) {
//     // Queue is full, build a new one
//     _garbage.emplace_back(std::exchange(buf, buf->resize(b, t)));
//     _buffer.store(buf, relaxed);
//   }

//   // Construct new object, this does not have to be atomic as no one can steal
//   // this item until after we store the new value of bottom, ordering is
//   // maintained by surrounding atomics.
//   buf->store(b, std::move(object));

//   std::atomic_thread_fence(release);
//   _bottom.store(b + 1, relaxed);
// }

// template <Trivial T>
// std::optional<T> Dequeue<T>::pop() noexcept {
//   std::int64_t b = _bottom.load(relaxed) - 1;
//   detail::ring_buf<T>* buf = _buffer.load(relaxed);

//   _bottom.store(b, relaxed);  // Stealers can no longer steal

//   std::atomic_thread_fence(seq_cst);
//   std::int64_t t = _top.load(relaxed);

//   if (t <= b) {
//     // Non-empty dequeue
//     if (t == b) {
//       // The last item could get stolen, by a stealer that loaded bottom before
//       // our write above
//       if (!_top.compare_exchange_strong(t, t + 1, seq_cst, relaxed)) {
//         // Failed race, thief got the last item.
//         _bottom.store(b + 1, relaxed);
//         return std::nullopt;
//       }
//       _bottom.store(b + 1, relaxed);
//     }

//     // Can delay load until after acquiring slot as only this thread can push(),
//     // this load is not required to be atomic as we are the exclusive writer.
//     return buf->load(b);

//   } else {
//     _bottom.store(b + 1, relaxed);
//     return std::nullopt;
//   }
// }

// template <Trivial T>
// std::optional<T> Dequeue<T>::steal() noexcept {
//   std::int64_t t = _top.load(acquire);
//   std::atomic_thread_fence(seq_cst);
//   std::int64_t b = _bottom.load(acquire);

//   if (t < b) {
//     // Must load *before* acquiring the slot as slot may be overwritten
//     // immediately after acquiring. This load is NOT required to be atomic
//     // even-though it may race with an overwrite as we only return the value if
//     // we win the race below garanteeing we had no race during our read. If we
//     // loose the race then 'x' could be corrupt due to read-during-write race
//     // but as T is trivially destructible this does not matter.
//     T x = _buffer.load(consume)->load(t);

//     if (!_top.compare_exchange_strong(t, t + 1, seq_cst, relaxed)) {
//       // Failed race.
//       return std::nullopt;
//     }

//     return x;

//   } else {
//     // Empty dequeue.
//     return std::nullopt;
//   }
// }

// template <Trivial T>
// Dequeue<T>::~Dequeue() noexcept {
//   delete _buffer.load();
// }

// }  // namespace fp