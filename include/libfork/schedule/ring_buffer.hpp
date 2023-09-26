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
#include <optional>
#include <utility>

#include "libfork/core/macro.hpp"
#include "libfork/core/utility.hpp"

/**
 * \file ring_buffer.hpp
 *
 * @brief A simple ring-buffer with customizable behavior on overflow/underflow.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A fixed capacity, power-of-two FILO ring-buffer with customizable behavior on overflow/underflow.
 */
template <std::default_initializable T, std::size_t N>
  requires (std::has_single_bit(N))
class ring_buffer {

  struct discard {
    LF_STATIC_CALL constexpr auto operator()(T const &) LF_STATIC_CONST noexcept -> bool { return false; }
  };

 public:
  /**
   * @brief Test whether the ring-buffer is empty.
   */
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return m_top == m_bottom; }

  /**
   * @brief Test whether the ring-buffer is full.
   */
  [[nodiscard]] constexpr auto full() const noexcept -> bool { return m_bottom - m_top == N; }

  /**
   * @brief Pushes a value to the ring-buffer.
   *
   * If the buffer is full then calls `when_full` with the value and returns false, otherwise returns true.
   * By default, `when_full` is a no-op.
   */
  template <std::invocable<T const &> F = discard>
  constexpr auto push(T const &val, F &&when_full = {}) noexcept(std::is_nothrow_invocable_v<F, T const &>)
      -> bool {
    if (full()) {
      std::invoke(std::forward<F>(when_full), val);
      return false;
    }
    store(m_bottom++, val);
    return true;
  }

  /**
   * @brief Pops (removes and returns) the last value pushed into the ring-buffer.
   *
   * If the buffer is empty calls `when_empty` and returns the result. By default, `when_empty` is a no-op
   * that returns a null `std::optional<T>`.
   */
  template <std::invocable F = impl::return_nullopt<T>>
    requires std::convertible_to<T &, std::invoke_result_t<F>>
  constexpr auto pop(F &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<F>)
      -> std::invoke_result_t<F> {
    if (empty()) {
      return std::invoke(std::forward<F>(when_empty));
    }
    return load(--m_bottom);
  }

 private:
  auto store(std::size_t index, T const &val) noexcept -> void {
    m_buff[index & mask] = val; // NOLINT
  }

  [[nodiscard]] auto load(std::size_t index) noexcept -> T & {
    return m_buff[(index & mask)]; // NOLINT
  }

  static constexpr std::size_t mask = N - 1;

  std::size_t m_top = 0;
  std::size_t m_bottom = 0;
  std::array<T, N> m_buff;
};

} // namespace ext

} // namespace lf

#endif /* C0E5463D_72D1_43C1_9458_9797E2F9C033 */
