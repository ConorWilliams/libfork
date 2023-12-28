#ifndef F51F8998_9E69_458E_95E1_8592A49FA76C
#define F51F8998_9E69_458E_95E1_8592A49FA76C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <new>

#include "libfork/core/impl/utility.hpp"

/**
 * @file manual_lifetime.hpp
 *
 * @brief A utility class for explicitly managing the lifetime of an object.
 */

namespace lf::impl {

// TODO: we could make manual_lifetime<T> empty if T is empty?

/**
 * @brief Provides storage for a single object of type ``T``.
 *
 * Every instance of manual_lifetime is trivially constructible/destructible.
 */
template <typename T>
class manual_lifetime : immovable<manual_lifetime<T>> {
 public:
  /**
   * @brief Start lifetime of object.
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  auto construct(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> T * {
    return ::new (static_cast<void *>(m_buf.data())) T(std::forward<Args>(args)...);
  }

  /**
   * @brief Start lifetime of object at assignment.
   */
  template <typename U>
    requires std::constructible_from<T, U>
  void operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) {
    this->construct(std::forward<U>(expr));
  }

  /**
   * @brief Destroy the contained object, must have been constructed first.
   *
   * A noop if ``T`` is trivially destructible.
   */
  void destroy() noexcept(std::is_nothrow_destructible_v<T>)
    requires std::is_destructible_v<T>
  {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      std::destroy_at(data());
    }
  }

  /**
   * @brief Get a pointer to the contained object, must have been constructed first.
   */
  [[nodiscard]] auto data() noexcept -> T * { return std::launder(reinterpret_cast<T *>(m_buf.data())); }

  /**
   * @brief Get a pointer to the contained object, must have been constructed first.
   */
  [[nodiscard]] auto data() const noexcept -> T * {
    return std::launder(reinterpret_cast<T const *>(m_buf.data()));
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() noexcept -> T * { return data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() const noexcept -> T const * { return data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() & noexcept -> T & { return *data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const & noexcept -> T const & { return *data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() && noexcept -> T && { return std::move(*data()); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T const && { return std::move(*data()); }

 private:
  [[no_unique_address]] alignas(T) std::array<std::byte, sizeof(T)> m_buf;
};

} // namespace lf::impl

#endif /* F51F8998_9E69_458E_95E1_8592A49FA76C */
