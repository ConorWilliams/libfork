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
 * @file immediate.hpp
 *
 * @brief An execution context that runs tasks inline..
 */

namespace lf {

/**
 * @brief An scheduler context that runs tasks inline.
 */
class immediate {
 public:
  /**
   * @brief The context type for the the immediate scheduler.
   */
  class context {
   public:
    /**
     * @brief Push a task to the context's stack.
     */
    auto push(task_handle<context> task) -> void { m_tasks.push(task); }
    /**
     * @brief Pop a task from the context's stack.
     */
    auto pop() -> std::optional<task_handle<context>> {
      if (m_tasks.empty()) {
        return std::nullopt;
      }
      auto task = m_tasks.top();
      m_tasks.pop();
      return task;
    }

   private:
    friend class immediate;

    std::stack<task_handle<context>, std::vector<task_handle<context>>> m_tasks;
  };

  static_assert(::lf::context<context>);

  /**
   * @brief Run a task to completion.
   */
  template <typename T, typename Allocator>
  auto schedule(basic_task<T, context, Allocator>&& task) -> T {
    //
    auto [fut, handle] = std::move(task).make_promise();

    handle.resume_root(m_context);

    if constexpr (!std::is_void_v<T>) {
      return *std::move(fut);
    } else {
      return;
    }
  }

  /**
   * @brief Check if the context's stack is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool { return m_context.m_tasks.empty(); }

 private:
  context m_context;
};

}  // namespace lf