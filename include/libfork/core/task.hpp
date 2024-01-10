#ifndef AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172
#define AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for movable
#include <type_traits> // for type_identity

#include "libfork/core/macro.hpp" // for LF_CORO_ATTRIBUTES

/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

inline namespace core {

// --------------------------------- Task --------------------------------- //

// TODO: private destructor such that tasks can only be created inside the library?

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` a reference or a `std::movable` type.
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
 *    The value type ``T`` of a coroutine should be independent of the coroutines first-argument.
 *
 * \endrst
 */
template <returnable T = void>
struct LF_CORO_ATTRIBUTES task : std::type_identity<T> {
  void *prom; ///< An opaque handle to the coroutine promise.
};

} // namespace core

} // namespace lf

#endif /* AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172 */
