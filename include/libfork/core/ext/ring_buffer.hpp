#ifndef C0E5463D_72D1_43C1_9458_9797E2F9C033
#define C0E5463D_72D1_43C1_9458_9797E2F9C033

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "libfork/core/macro.hpp"

#include "libfork/core/impl/utility.hpp"

/**
 * \file ring_buffer.hpp
 *
 * @brief A simple ring-buffer with customizable behavior on overflow/underflow.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A fixed (power-of-two) capacity, ring-buffer backed container.
 *
 * A `ring_buffer` stores its elements inline like a `std::array`.
 */
template <std::movable T, std::size_t N>
  requires (std::has_single_bit(N))
class ring_buffer {
 public:
  /**
   * @brief Capacity of the ring buffer.
   */
  [[nodiscard]] static constexpr auto capacity() noexcept -> std::size_t { return N; }

  /**
   * @brief Number of elements in the ring buffer.
   */
  [[nodiscard]] constexpr auto size() noexcept -> std::size_t { return m_back - m_front; }

  /**
   * @brief Test whether the ring-buffer is empty.
   */
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return m_back == m_front; }

  /**
   * @brief Test whether the ring-buffer is full.
   */
  [[nodiscard]] constexpr auto full() const noexcept -> bool { return size() == capacity(); }

  /**
   * @brief Add element to the front of the ring buffer, overwriting the back element if full.
   *
   * This may either construct a new element in place or assign to an existing element.
   */
  template <typename U>
    requires std::constructible_from<T, U> && std::is_assignable_v<T &, U>
  constexpr auto push_font(U &&val) {
    if (full()) {
      *load(m_front - 1) = std::forward<U>(val);
      m_front--;
      m_back--;
      return;
    }
    load(m_front - 1).construct(std::forward<U>(val));
    m_front--;
    return;
  }

  /**
   * @brief Add element to the back of the ring buffer, overwriting the front element if full.
   *
   * This may either construct a new element in place or assign to an existing element.
   */
  template <typename U>
    requires std::constructible_from<T, U> && std::is_assignable_v<T &, U>
  constexpr auto push_back(U &&val) {
    if (full()) {
      *load(m_back) = std::forward<U>(val);
      m_front++;
      m_back++;
      return;
    }
    load(m_back).construct(std::forward<U>(val));
    m_front++;
    return;
  }

  /**
   * @brief Remove and return the back element of the ring buffer.
   *
   * The buffer must not be empty.
   */
  constexpr auto pop_back() noexcept -> T
    requires std::is_nothrow_move_constructible_v<T>
  {
    LF_ASSERT(!empty());

    auto &slot = load(m_back - 1);

    impl::defer at_exit = [&]() noexcept {
      slot.destruct();
      m_back--;
    };

    return std::move(*slot);
  }

  /**
   * @brief Remove and return the front element of the ring buffer.
   *
   * The buffer must not be empty.
   */
  constexpr auto pop_front() noexcept -> T
    requires std::is_nothrow_move_constructible_v<T>
  {
    LF_ASSERT(!empty());

    auto &slot = load(m_front);

    impl::defer at_exit = [&]() noexcept {
      slot.destruct();
      m_front++;
    };

    return std::move(*slot);
  }

 private:
  [[nodiscard]] auto load(std::size_t index) noexcept -> impl::manual_lifetime<T> & {
    return m_buff[(index & mask)]; // NOLINT
  }

  static constexpr std::size_t mask = N - 1;

  std::unique_ptr<impl::manual_lifetime<T>[]> m_buff = std::make_unique<impl::manual_lifetime<T>[]>(N);

  std::size_t m_front = std::numeric_limits<std::size_t>::max() / 2;
  std::size_t m_back = std::numeric_limits<std::size_t>::max() / 2;
};

} // namespace ext

} // namespace lf

#endif /* C0E5463D_72D1_43C1_9458_9797E2F9C033 */
