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

#include "libfork/core/impl/utility.hpp"

/**
 * @file eventually.hpp
 *
 * @brief A class for delaying construction of an object.
 */

namespace lf {

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
 *    If you are placing instances of `eventually` on the heap you need to be very careful about exceptions.
 *
 * \endrst
 */
template <impl::non_void T>
class eventually;

// ------------------------------------------------------------------------ //

// TODO: stop references binding to temporaries

// ----- //

template <typename To, typename From>
struct safe_ref_bind_impl : std::false_type {};

// All reference types can bind to a non-dangling reference of the same kind without dangling.

template <typename T>
struct safe_ref_bind_impl<T, T> : std::true_type {};

// T const X can additionally bind to U X without dangling//

template <typename To, typename From>
struct safe_ref_bind_impl<To const &, From &> : std::true_type {};

template <typename To, typename From>
struct safe_ref_bind_impl<To const &&, From &&> : std::true_type {};

template <impl::non_void T>
  requires impl::reference<T>
class eventually<T> : impl::immovable<eventually<T>> {
 public:
  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <typename U>
    requires std::same_as<T, U &&> || std::same_as<T, impl::constify_ref_t<U &&>>
  constexpr auto operator=(U &&expr) noexcept -> eventually & {
    m_value = std::addressof(expr);
    return *this;
  }

  /**
   * @brief Access the wrapped object.
   *
   * This will decay T&& to T& just like a regular T&& function parameter.
   */
  [[nodiscard]] constexpr auto operator*() const & noexcept -> std::remove_reference_t<T> & {
    return *impl::non_null(m_value);
  }

  /**
   * @brief Access the wrapped object as is.
   *
   * This will not decay T&& to T&, nor will it promote T& to T&&.
   */
  [[nodiscard]] constexpr auto operator*() const && noexcept -> T {
    if constexpr (std::is_rvalue_reference_v<T>) {
      return std::move(*impl::non_null(m_value));
    } else {
      return *impl::non_null(m_value);
    }
  }

 private:
  std::remove_reference_t<T> *m_value = nullptr;
};

// ------------------------------------------------------------------------ //

// ------------------------------------------------------------------------ //

template <impl::non_void T>
class eventually : impl::immovable<eventually<T>> {
 public:
  // clang-format off

  /**
   * @brief Construct an empty eventually.
   */
  constexpr eventually() noexcept requires std::is_trivially_constructible_v<T> = default;

  // clang-format on

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS
  constexpr eventually() noexcept : m_init{} {}
#endif

  /**
   * @brief Construct an object inside the eventually as if by ``T(args...)``.
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr void emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    LF_LOG("Constructing an eventually");
#ifndef NDEBUG
    LF_ASSERT(!m_constructed);
#endif
    std::construct_at(std::addressof(m_value), std::forward<Args>(args)...);
#ifndef NDEBUG
    m_constructed = true;
#endif
  }

  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <typename U>
  constexpr auto operator=(U &&expr) noexcept(noexcept(emplace(std::forward<U>(expr)))) -> eventually &
    requires requires (eventually self) { self.emplace(std::forward<U>(expr)); }
  {
    emplace(std::forward<U>(expr));
    return *this;
  }

  // clang-format off

  /**
   * @brief Destroy the object which __must__ be inside the eventually.
   */
  constexpr ~eventually() noexcept requires std::is_trivially_destructible_v<T> = default;

  // clang-format on

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

  constexpr ~eventually() noexcept(std::is_nothrow_destructible_v<T>) {
  #ifndef NDEBUG
    LF_ASSUME(m_constructed);
  #endif
    std::destroy_at(std::addressof(m_value));
  }

#endif

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() & noexcept -> T & {
#ifndef NDEBUG
    LF_ASSUME(m_constructed);
#endif
    return m_value;
  }

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() && noexcept(std::is_nothrow_move_constructible_v<T>) -> T {
#ifndef NDEBUG
    LF_ASSUME(m_constructed);
#endif
    return std::move(m_value);
  }

 private:
  union {
    impl::empty m_init;
    T m_value;
  };

#ifndef NDEBUG
  bool m_constructed = false;
#endif
};

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */
