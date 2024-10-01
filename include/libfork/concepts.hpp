#ifndef DD0B4328_55BD_452B_A4A5_5A4670A6217B
#define DD0B4328_55BD_452B_A4A5_5A4670A6217B

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for invocable, constructible_from, convertible_to
#include <functional>  // for invoke
#include <type_traits> // for invoke_result_t, remove_cvref_t, false_type
#include <utility>     // for forward

/**
 * @file first_arg.hpp
 *
 * @brief Machinery for the (library-generated) first argument of async functions.
 */

namespace lf {

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` a reference or a `std::movable` type.
 */
template <typename T>
concept returnable = std::is_void_v<T> || std::is_reference_v<T> || std::movable<T>;

/**
 * @brief Test if we can form a reference to an instance of type `T`.
 */
template <typename T>
concept referenceable = requires () { typename std::type_identity_t<T &>; };

/**
 * @brief Test if the expression `*std::declval<T&>()` is valid and is `lf::core::referenceable`.
 */
template <typename I>
concept dereferenceable = requires (I val) {
  { *val } -> referenceable;
};

namespace detail {

/**
 * @brief Test if a type could be a quasi-pointer.
 *
 * @tparam I The unqualified type.
 * @tparam Qual The Qualified version of `I`.
 */
template <typename I, typename Qual>
concept quasi_pointer_impl =                                //
    std::default_initializable<I> &&                        //
    std::movable<I> &&                                      //
    dereferenceable<I> && std::constructible_from<I, Qual>; //

} // namespace detail

/**
 * @brief A quasi-pointer if a movable type that can be dereferenced to a `lf::core::referenceable`.
 *
 * A quasi-pointer is assumed to be cheap-to-move like an iterator/legacy-pointer.
 */
template <typename I>
concept quasi_pointer = detail::quasi_pointer_impl<std::remove_cvref_t<I>, I>;

/**
 * @brief A concept that checks if a quasi-pointer can be used to stash an exception.
 *
 * If the expression `stash_exception(*ptr)` is well-formed and `noexcept` and `ptr` is
 * used as the return address of an async function, then if that function terminates
 * with an exception, the exception will be stored in the quasi-pointer via a call to
 * `stash_exception`.
 */
template <typename I>
concept stash_exception_in_return = quasi_pointer<I> && requires (I ptr) {
  { stash_exception(*ptr) } noexcept;
};

/**
 * @brief A type which can be assigned any value as a noop.
 *
 * Useful to ignore a value tagged with ``[[no_discard]]``.
 */
struct ignore_t {
  /**
   * @brief A no-op assignment operator.
   */
  constexpr auto operator=(auto const & /* unused */) const noexcept -> ignore_t const & { return *this; }
};

/**
 * @brief A tag type to indicate an async function's return value will be discarded by the caller.
 *
 * This type is indirectly writable from any value.
 */
struct discard_t {
  /**
   * @brief Return a proxy object that can be assigned any value.
   */
  constexpr auto operator*() -> ignore_t { return {}; }
};

namespace detail {

// Base case: invalid
template <typename I, typename R>
struct valid_return : std::false_type {};

// Special case: discard_t is valid for void
template <>
struct valid_return<discard_t, void> : std::true_type {};

// Special case: stash_exception_in_return + void
template <stash_exception_in_return I>
struct valid_return<I, void> : std::true_type {};

// Anything indirectly_writable
// template <returnable R, std::indirectly_writable<R> I>
// struct valid_return<I, R> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that `R` is returned via `I`.
 *
 * This requires that `I` is `std::indirectly_writable` or that `I` is `discard_t` and the task returns void.
 */
template <typename I, typename R>
inline constexpr bool valid_return_v = quasi_pointer<I> && detail::valid_return<I, R>::value;

/**
 * @brief Verify that `R` returned via `I`.
 *
 * This requires that `I` is `std::indirectly_writable` or that `I` is `discard_t` and the `R` is void.
 */
template <typename I, typename R>
concept return_address_for = quasi_pointer<I> && returnable<R> && valid_return_v<I, R>;

namespace detail {

template <typename To, typename From>
struct safe_ref_bind_impl : std::false_type {};

// All reference types can bind to a non-dangling reference of the same kind without dangling.

template <typename T>
struct safe_ref_bind_impl<T, T> : std::true_type {};

// `T const X` can additionally bind to `T X` without dangling.

template <typename To, typename From>
  requires (!std::same_as<To const &, From &>)
struct safe_ref_bind_impl<To const &, From &> : std::true_type {};

template <typename To, typename From>
  requires (!std::same_as<To const &&, From &&>)
struct safe_ref_bind_impl<To const &&, From &&> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that ``To expr = From`` is valid and does not dangle.
 *
 * This requires that ``To`` and ``From`` are both the same reference type or that ``To`` is a
 * const qualified version of ``From``. This explicitly bans conversions like ``T && -> T const &``
 * which would dangle.
 */
template <typename From, typename To>
concept safe_ref_bind_to =                          //
    std::is_reference_v<To> &&                      //
    referenceable<From> &&                          //
    detail::safe_ref_bind_impl<To, From &&>::value; //

/**
 * @brief Check is a type is not ``void``.
 */
template <typename T>
concept non_void = !std::is_void_v<T>;

} // namespace lf

#endif /* DD0B4328_55BD_452B_A4A5_5A4670A6217B */