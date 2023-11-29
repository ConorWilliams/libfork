#ifndef AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172
#define AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>

#include "libfork/core/tag.hpp"

/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

// --------------------------------- Task --------------------------------- //

// TODO: private destructor such that tasks can only be created inside the library?

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 */
template <typename T>
concept returnable = std::is_void_v<T> || std::is_reference_v<T> || std::movable<T>;

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other coroutines and specify `T` the
 * async function's return type which is required to be `void`, a reference, or a `std::movable` type.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should never touch an instance of this type, it is used for specifying the
 *    return type of an `async` function only.
 *
 * .. warning::
 *    The value type ``T`` of a coroutine should never be independent of the coroutines first-argument.
 *
 * \endrst
 */
template <returnable T = void>
struct task : std::type_identity<T> {
  void *promise; ///< An opaque handle to the coroutine promise.
};

// TODO [[clang::coro_only_destroy_when_complete]] [[clang::coro_return_type]]

namespace impl {

namespace detail {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

} // namespace detail

/**
 * @brief Test if a type is a specialization of ``lf::task``.
 *
 * This does not accept cv-qualified or reference types.
 */
template <typename T>
inline constexpr bool is_task_v = detail::is_task_impl<T>::value;

} // namespace impl

} // namespace lf

#endif /* AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172 */
