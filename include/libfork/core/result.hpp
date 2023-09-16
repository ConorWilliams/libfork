#ifndef EE6A2701_7559_44C9_B708_474B1AE823B2
#define EE6A2701_7559_44C9_B708_474B1AE823B2

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <semaphore>
#include <tuple>
#include <type_traits>
#include <utility>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core/eventually.hpp"

namespace lf {

/**
 * @brief A small control structure that a root tasks use to communicate with the main thread.
 */
template <typename T>
struct root_result;

template <>
struct root_result<void> : detail::immovable<root_result<void>> {
  std::binary_semaphore semaphore{0};
};

template <typename T>
struct root_result : eventually<T>, root_result<void> {
  using eventually<T>::operator=;
};

namespace detail {

template <typename T>
struct is_root_result : std::false_type {};

template <typename T>
struct is_root_result<root_result<T>> : std::true_type {};

} // namespace detail

template <typename T>
inline constexpr bool is_root_result_v = detail::is_root_result<T>::value;

// ------------------------------------------------------------------------ //

/**
 * @brief A tuple-like type with forwarding semantics for in place construction.
 */
template <typename... Args>
struct in_place : std::tuple<Args...> {
  using std::tuple<Args...>::tuple;
};

/**
 * @brief A forwarding deduction guide.
 */
template <typename... Args>
in_place(Args &&...) -> in_place<Args &&...>;

// ------------------------------------------------------------------------ //

namespace detail {

// General case = invalid.
template <typename R, typename T>
struct valid_result_help : std::false_type {};

// Ignore case
template <typename T>
struct valid_result_help<void, T> : std::true_type {};

// Root result special case (especially T = void)
template <typename T>
struct valid_result_help<root_result<T>, T> : std::true_type {};

// Eventually special (for immovable types that cannot be assigned).
template <typename T>
struct valid_result_help<eventually<T>, T> : std::true_type {};

template <typename R, typename T>
  requires std::is_assignable_v<R &, T>
struct valid_result_help<R, T> : std::true_type {};

} // namespace detail

template <typename R, typename T>
concept valid_result = !reference<R> && detail::valid_result_help<R, T>::value;

// ------------------------------------------------------------------------ //

template <typename T>
struct maybe_ptr {
  explicit constexpr maybe_ptr(T *ptr) noexcept : m_ptr(non_null(ptr)) {}

  constexpr auto address() const noexcept -> T * { return m_ptr; }

private:
  T *m_ptr;
};

template <>
struct maybe_ptr<void> {};

// ----------------------- //

/**
 * @brief A base class for promises that provides the return_[...] methods.
 *
 * @tparam R The type of the return address.
 * @tparam T The type of the return value.
 */
template <typename R, typename T>
  requires valid_result<R, T>
struct promise_result;

// ------------------------------ void/ignore ------------------------------ //

template <>
struct promise_result<void, void> {
  static constexpr void return_void() noexcept { LF_LOG("return void"); }
};

// ------------------------------ rooted void ------------------------------ //

template <>
struct promise_result<root_result<void>, void> : maybe_ptr<root_result<void>> {

  using maybe_ptr<root_result<void>>::maybe_ptr;

  static constexpr void return_void() noexcept { LF_LOG("return void"); }
};

// ------------------------------ general case ------------------------------ //

/*

Want to model:

R val = [] -> T { ... }();

But really have:

R val;

val = [] -> T { ... }();

*/

template <typename T, typename U>
concept converting = !std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>;

template <typename R, typename T>
  requires valid_result<R, T>
struct promise_result : maybe_ptr<R> {

  using maybe_ptr<R>::maybe_ptr;

  /**
   * @brief Assign `value` to the return address.
   */
  constexpr void return_value(T const &value) const
    requires std::convertible_to<T const &, T> && (not reference<T>)
  {
    if constexpr (non_void<R>) {
      *(this->address()) = value;
    }
  }

  /**
   * @brief Move assign `value` to the return address.
   */
  constexpr void return_value(T &&value) const
    requires std::convertible_to<T &&, T>
  {
    if constexpr (non_void<R>) {
      if constexpr (std::is_rvalue_reference_v<T &&>) {
        *(this->address()) = std::move(value);
      } else {
        *(this->address()) = value;
      }
    }
  }

  /**
   * @brief Assign `value` to the return address.
   *
   * If the return address is directly assignable from `value` this will not construct the intermediate `T`.
   */
  template <std::convertible_to<T> U>
    requires converting<T, U>
  constexpr void return_value(U &&value) const {
    if constexpr (non_void<R>) {
      if constexpr (std::is_assignable_v<R &, U &&>) {
        *(this->address()) = std::forward<U>(value);
      } else {
        *(this->address()) = [&]() -> T { return std::forward<U>(value); }();
      }
    }
  }

private:
  template <typename U>
  using strip_rvalue_ref_t = std::conditional_t<std::is_rvalue_reference_v<U>, std::remove_reference_t<U>, U>;

public:
  /**
   * @brief Assign a value constructed from the arguments stored in `args` to the return address.
   *
   * If the return address has an `.emplace()` method that accepts the arguments in the tuple this will be
   * called directly.
   */
  template <reference... Args>
    requires std::constructible_from<T, Args...>
  constexpr void return_value(in_place<Args...> args) const {

#define LF_FWD_ARGS std::forward<strip_rvalue_ref_t<Args>>(args)...

    if constexpr (non_void<R>) {
      apply_to(static_cast<std::tuple<Args...> &&>(args), [ret = this->address()](Args... args) {
        if constexpr (requires { ret->emplace(LF_FWD_ARGS); }) {
          ret->emplace(LF_FWD_ARGS);
        } else {
          (*ret) = T(LF_FWD_ARGS);
        }
      });
    }
  }

#undef LF_FWD_ARGS
};

// ----------------------------------------------------- //

} // namespace lf

#endif /* EE6A2701_7559_44C9_B708_474B1AE823B2 */
