#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stack>
#include <thread>

#include "libfork/queue.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"
/**
 * @file inline.hpp
 *
 * @brief An execution context that runs tasks inline..
 */

namespace lf {
/**
 * @brief An execution context that runs tasks inline.
 */
struct inline_context : queue<task_handle<inline_context>> {
  /**
   * @brief Submit a root task to the pool.
   */
  void submit(task_handle<inline_context> task) noexcept { task.resume_root(*this); }
};

static_assert(context<inline_context>);

}  // namespace lf