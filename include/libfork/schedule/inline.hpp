#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stack>
#include <thread>
#include <vector>

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
class inline_context {
 public:
  /**
   * @brief Push a task to the context's stack.
   */
  auto push(task_handle<inline_context> task) -> void { m_tasks.push(task); }
  /**
   * @brief Pop a task from the context's stack.
   */
  auto pop() -> std::optional<task_handle<inline_context>> {
    if (m_tasks.empty()) {
      return std::nullopt;
    }
    auto task = m_tasks.top();
    m_tasks.pop();
    return task;
  }
  /**
   * @brief Check if the context's stack is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool { return m_tasks.empty(); }

 private:
  std::stack<task_handle<inline_context>, std::vector<task_handle<inline_context>>> m_tasks;
};

static_assert(context<inline_context>);

}  // namespace lf