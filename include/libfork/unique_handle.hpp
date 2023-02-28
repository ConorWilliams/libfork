#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>

#include "libfork/detail/coroutine.hpp"
#include "libfork/detail/utility.hpp"

/**
 * @file unique_handle.hpp
 *
 * @brief Implementation of the ``unique_handle`` class.
 */

namespace lf {

/**
 * @brief A RAII handle to a ``std::coroutine_handle``
 */
template <typename P>
class unique_handle {
 public:
  /**
   * @brief Construct an empty handle.
   */
  constexpr unique_handle() noexcept : m_data{nullptr} {}
  /**
   * @brief Construct a ``unique_handle`` to manage ``handle``.
   */
  explicit constexpr unique_handle(coroutine_handle<P> handle) noexcept : m_data{handle} {}
  /**
   * @brief A ``unique_handle`` cannot be copied.
   */
  unique_handle(unique_handle const&) = delete;
  /**
   * @brief Move construct a ``unique_handle`` from ``other``.
   */
  constexpr unique_handle(unique_handle&& other) noexcept : m_data{std::exchange(other.m_data, nullptr)} {}
  /**
   * @brief A ``unique_handle`` cannot be copied.
   */
  auto operator=(unique_handle const&) -> unique_handle& = delete;
  /**
   * @brief Move assign a ``unique_handle`` from ``other``.
   */
  constexpr auto operator=(unique_handle&& other) noexcept -> unique_handle& {
    if (coroutine_handle old = std::exchange(m_data, other.m_data)) {
      DEBUG_TRACKER("destroying a coroutine");
      old.destroy();
    }
    return *this;
  }
  /**
   * @brief Test if the underlying handle is non-null.
   */
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return static_cast<bool>(m_data); }
  /**
   * @brief Access the underlying handle.
   */
  constexpr auto operator->() const noexcept -> coroutine_handle<P> const* { return std::addressof(m_data); }
  /**
   * @brief Access the underlying handle.
   */
  [[nodiscard]] constexpr auto operator*() const noexcept -> coroutine_handle<P> { return m_data; }

  /**
   * @brief Release ownership of the underlying handle.
   */
  constexpr void release() noexcept {
    DEBUG_TRACKER("releasing a coroutine");
    m_data = nullptr;
  }

  /**
   * @brief Destroy the handle.
   */
  constexpr ~unique_handle() noexcept {
    if (coroutine_handle old = std::exchange(m_data, nullptr)) {
      DEBUG_TRACKER("destroying a coroutine");
      old.destroy();
    }
  }

 private:
  coroutine_handle<P> m_data;
};

}  // namespace lf