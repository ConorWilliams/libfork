#ifndef AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172
#define AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for movable
#include <type_traits> // for type_identity

#include "libfork/concepts.hpp"
#include "libfork/detail/macro.hpp" // for LF_CORO_ATTRIBUTES
#include "libfork/eventually.hpp"

/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

// --------------------------------- Task --------------------------------- //

namespace detail {

template <typename T>
struct default_return_for_impl : std::type_identity<T *> {};

template <typename T>
  requires std::is_reference_v<T>
struct default_return_for_impl<T> : std::type_identity<eventually<std::remove_reference_t<T>> *> {};

template <>
struct default_return_for_impl<void> : std::type_identity<ignore_t> {};

} // namespace detail

/**
 * @brief Generate the default quasi-pointer for a given type.
 *
 * The following rules are used to determine the default return type:
 *  - If `T` is `void` then the return pointer is 'discared_t`.
 *  - If `T` is a reference type then the return pointer is `eventually<T> *`.
 *  - Otherwise the return pointer is `T *`.
 */
template <returnable T>
using default_return_for_t = typename detail::default_return_for_impl<T>::type;

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other
 * coroutines and specify `T` the async function's return type which is required
 * to be `void`, a reference, or a `std::movable` type.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should ever touch an instance of this type, it
 *    is used for specifying the return type of an `async` function only.
 *
 * \endrst
 */
template <returnable T = void, return_address_for<T> = default_return_for_t<T>>
struct LF_CORO_ATTRIBUTES task {};

} // namespace lf

#endif /* AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172 */