#ifndef ACB944D8_08B6_4600_9302_602E847753FD
#define ACB944D8_08B6_4600_9302_602E847753FD

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits> // for is_standard_layout_v
#include <version>     // for __cpp_lib_is_pointer_interconvertible_base_of

#include "libfork/core/impl/frame.hpp" // for frame

/**
 * @file handles.hpp
 *
 * @brief Definitions of `libfork`'s handle types.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A type safe wrapper around a handle to a coroutine that is at a submission point.
 *
 * Instances of this type (wrapped in an `lf::intrusive_list`s node) will be passed to a worker's context.
 *
 * \rst
 *
 * .. note::
 *
 *    A pointer to an ``submit_h`` should never be deferenced, only passed to ``lf::ext::resume()``.
 *
 * \endrst
 */
class submit_t : impl::frame {};

static_assert(std::is_standard_layout_v<submit_t>);

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_base_of_v<impl::frame, submit_t>);
#endif

/**
 * @brief An alias for a pointer to a `submit_t`.
 */
using submit_handle = submit_t *;

/**
 * @brief A type safe wrapper around a handle to a stealable coroutine.
 *
 * Instances of this type will be passed to a worker's context.
 *
 * \rst
 *
 * .. note::
 *
 *    A pointer to an ``task_h`` should never be deferenced, only passed to ``lf::ext::resume()``.
 *
 * \endrst
 */
class task_t : impl::frame {};

static_assert(std::is_standard_layout_v<task_t>);

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_base_of_v<impl::frame, task_t>);
#endif

/**
 * @brief An alias for a pointer to a `task_t`.
 */
using task_handle = task_t *;

} // namespace ext

} // namespace lf

#endif /* ACB944D8_08B6_4600_9302_602E847753FD */
