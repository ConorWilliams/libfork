#ifndef A75DC3F0_D0C3_4669_A901_0B22556C873C
#define A75DC3F0_D0C3_4669_A901_0B22556C873C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * @file tag.hpp
 *
 * @brief Libfork's dispatch tag types.
 */

namespace lf {

inline namespace core {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 *
 * You can inspect the first argument of an async function to determine the tag.
 */
enum class tag {
  root, ///< This coroutine is a root task from an ``lf::sync_wait``.
  call, ///< Non root task from an ``lf::call``, completes synchronously.
  fork, ///< Non root task from an ``lf::fork``, completes asynchronously.
};

} // namespace core

} // namespace lf

#endif /* A75DC3F0_D0C3_4669_A901_0B22556C873C */
