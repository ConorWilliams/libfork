#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#include "libfork/utility.hpp"

/**
 * @file result.hpp
 *
 * @brief Implementation of the ``result`` mixin class.
 */

namespace lf::detail {

/**
 * @brief A promise mixin that provides the result handling for a task.
 *
 * A result object MUST have ``return_value()`` called before it is destruced.
 */
template <typename T>
class result {
 public:
  /**
   * @brief Construct an empty result object.
   */
  constexpr result() noexcept : m_empty{} {}

  result(result&) = delete;

  result(result&&) = delete;

  auto operator=(result&) -> result& = delete;

  auto operator=(result&&) -> result& = delete;

  /**
   * @brief Return the result of the task.
   */
  [[nodiscard]] constexpr auto get() & noexcept -> T& { return m_value; }  // NOLINT
  /**
   * @brief Return the result of the task.
   */
  [[nodiscard]] constexpr auto get() const& noexcept -> T const& { return m_value; }  // NOLINT
  /**
   * @brief Return the result of the task.
   */
  [[nodiscard]] constexpr auto get() && noexcept -> T&& { return std::move(m_value); }  // NOLINT
  /**
   * @brief Return the result of the task.
   */
  [[nodiscard]] constexpr auto get() const&& noexcept -> T const&& { return std::move(m_value); }  // NOLINT
  /**
   * @brief Set the internal result member.
   */
  template <typename U, bool Except = std::is_nothrow_constructible_v<T, U&&>>
  requires std::constructible_from<T, U&&>
  constexpr auto return_value(U&& value) noexcept(Except) -> void {
    DEBUG_TRACKER("task returns value");
    std::construct_at(std::addressof(m_value), std::forward<U>(value));  // NOLINT
  }

  // clang-format off

  /// @cond CONCEPTS

  ~result() requires(std::is_trivially_destructible_v<T>) = default;

  /// @endcond

  // clang-format on

  constexpr ~result() noexcept {
    std::destroy_at(std::addressof(m_value));  // NOLINT
  }

 private:
  struct empty {};

  union {
    empty m_empty;
    T m_value;
  };
};

/**
 * @brief A promise mixin that provides result handling for reference types for a task.
 */
template <typename T>
requires std::is_reference_v<T>
class result<T> {
 public:
  constexpr result() noexcept = default;

  result(result&) = delete;

  result(result&&) = delete;

  auto operator=(result&) -> result& = delete;

  auto operator=(result&&) -> result& = delete;

  ~result() = default;

  /**
   * @brief Return the result of the task.
   */
  [[nodiscard]] constexpr auto get() const -> T { return *m_value; }

  /**
   * @brief Set the internal result member.
   */
  constexpr auto return_value(T value) noexcept -> void {
    DEBUG_TRACKER("task returns reference");
    ASSERT_ASSUME(!m_value, "return value called twice");
    m_value = std::addressof(value);
  }

 private:
  std::remove_reference_t<T>* m_value{};
};

/**
 * @brief A promise mixin specialization that provides result handling for void returning tasks.
 */
template <>
class result<void> {
 public:
  /**
   * @brief Noop for void returns.
   */
  static constexpr void return_void() noexcept { DEBUG_TRACKER("task returns void"); }
};

}  // namespace lf::detail