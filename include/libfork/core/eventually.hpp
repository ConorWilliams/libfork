#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for constructible_from
#include <optional>    // for optional
#include <type_traits> // for remove_reference_t
#include <utility>     // for addressof, forward

#include "libfork/core/impl/utility.hpp" // for immovable, non_void, safe_r...
#include "libfork/core/macro.hpp"        // for LF_ASSERT

/**
 * @file eventually.hpp
 *
 * @brief Classes for delaying construction of an object.
 */

namespace lf {

inline namespace core {

// ------------------------------------------------------------------------ //

/**
 * @brief A wrapper to delay construction of an object, like ``std::optional``.
 *
 * This class supports delayed construction of reference types. It is like a simplified version of
 * `std::optional` that is only constructed once. Construction is done (zero or one times) via assignment.
 *
 * \rst
 *
 * .. note::
 *    This documentation is generated from the non-reference specialization, see the source
 *    for the reference specialization.
 *
 * \endrst
 */
template <impl::non_void T>
class eventually : impl::immovable<eventually<T>> {

  std::optional<T> m_value; ///< The contained object.

 public:
  /**
   * @brief Start lifetime of object at assignment.
   */
  template <typename U>
    requires std::constructible_from<T, U>
  void operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) {
    LF_ASSERT(!m_value);
    m_value.emplace(std::forward<U>(expr));
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() noexcept -> T * {
    LF_ASSERT(m_value);
    return m_value.operator->();
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() const noexcept -> T const * {
    LF_ASSERT(m_value);
    return m_value.operator->();
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() & noexcept -> T & {
    LF_ASSERT(m_value);
    return *m_value;
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const & noexcept -> T const & {
    LF_ASSERT(m_value);
    return *m_value;
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() && noexcept -> T && {
    LF_ASSERT(m_value);
    return *std::move(m_value);
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T const && {
    LF_ASSERT(m_value);
    return *std::move(m_value);
  }
};

// ------------------------------------------------------------------------ //

/**
 * @brief Has pointer semantics.
 *
 * `eventually<T &> val` should behave like `T & val` except assignment rebinds.
 */
template <impl::non_void T>
  requires std::is_reference_v<T>
class eventually<T> : impl::immovable<eventually<T>> {
 public:
  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <impl::safe_ref_bind_to<T> U>
  void operator=(U &&expr) noexcept {
    m_value = std::addressof(expr);
  }

  /**
   * @brief Access the wrapped reference.
   */
  [[nodiscard]] auto operator->() const noexcept -> std::remove_reference_t<T> * { return m_value; }

  /**
   * @brief Deference the wrapped pointer.
   *
   * This will decay `T&&` to `T&` just like using a `T &&` reference would.
   */
  [[nodiscard]] auto operator*() const & noexcept -> std::remove_reference_t<T> & { return *m_value; }

  /**
   * @brief Forward the wrapped reference.
   *
   * This will not decay T&& to T&, nor will it promote T& to T&&.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T {
    if constexpr (std::is_rvalue_reference_v<T>) {
      return std::move(*m_value);
    } else {
      return *m_value;
    }
  }

 private:
  std::remove_reference_t<T> *m_value;
};

} // namespace core

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */
