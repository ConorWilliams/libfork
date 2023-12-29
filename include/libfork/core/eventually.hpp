#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "libfork/core/macro.hpp"
#include "libfork/core/task.hpp"

#include "libfork/core/impl/manual_lifetime.hpp"
#include "libfork/core/impl/utility.hpp"

/**
 * @file eventually.hpp
 *
 * @brief Classes for delaying construction of an object.
 */

namespace lf {

inline namespace core {

// ------------------------------------------------------------------------ //

/**
 * @brief A wrapper to delay construction of an object.
 *
 * This class supports delayed construction of immovable types and reference types.
 *
 * \rst
 *
 * .. note::
 *    This documentation is generated from the non-reference specialization, see the source
 *    for the reference specialization.
 *
 * .. warning::
 *    It is undefined behavior if the object inside an `eventually` is not constructed before it
 *    is used or if the lifetime of the ``lf::eventually`` ends before an object is constructed.
 *    If you are placing instances of `eventually` on the heap you need to be very careful about
 *    exceptions.
 *
 * \endrst
 */
template <impl::non_void T>
class eventually : impl::manual_lifetime<T> {
 public:
  using impl::manual_lifetime<T>::construct;
  using impl::manual_lifetime<T>::operator=;
  using impl::manual_lifetime<T>::operator->;
  using impl::manual_lifetime<T>::operator*;

  // clang-format off

  /**
   * @brief Destroy the object which __must__ be inside the eventually.
   */
  constexpr ~eventually() noexcept requires std::is_trivially_destructible_v<T> = default;

  // clang-format on

  constexpr ~eventually() noexcept(std::is_nothrow_destructible_v<T>) { this->destroy(); }
};

// ------------------------------------------------------------------------ //

/**
 * @brief Has pointer semantics.
 *
 * `eventually<T &> val` should behave like `T & val` except assignment rebinds.
 */
template <impl::non_void T>
  requires impl::reference<T>
class eventually<T> : impl::immovable<eventually<T>> {
 public:
  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <impl::safe_ref_bind_to<T> U>
  void construct(U &&expr) noexcept {
    m_value = std::addressof(expr);
  }

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

// ------------------------------------------------------------------------ //

/**
 * @brief A `lf::manual_eventually<T>` is an `lf::eventually<T>` which does not call destroy on destruction.
 *
 * This is useful for writing exception safe fork-join code and should be considered an expert-only feature.
 */
template <impl::non_void T>
class manual_eventually : impl::manual_lifetime<T> {

 public:
  using impl::manual_lifetime<T>::construct;
  using impl::manual_lifetime<T>::operator=;
  using impl::manual_lifetime<T>::operator->;
  using impl::manual_lifetime<T>::operator*;
  using impl::manual_lifetime<T>::destroy;
};

/**
 * @brief A `lf::manual_eventually<T>` is an `lf::eventually<T>` which does not call destroy on destruction.
 *
 * This is useful for writing exception safe fork-join code and should be considered an expert-only feature.
 */
template <impl::non_void T>
  requires impl::reference<T>
class manual_eventually<T> : eventually<T> {

 public:
  using eventually<T>::construct;
  using eventually<T>::operator=;
  using eventually<T>::operator->;
  using eventually<T>::operator*;

  /**
   * @brief Destroy the contained object (call its destructor).
   */
  void destroy() noexcept { static_assert(std::is_trivially_destructible_v<T>); };
};

} // namespace core

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */
