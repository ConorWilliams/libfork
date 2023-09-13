#ifndef C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC
#define C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "libfork/core.hpp"
#include "libfork/core/stack.hpp"

/**
 * @file inline.hpp
 *
 * @brief A scheduler that runs all tasks inline on current thread.
 */

namespace lf {

/**
 * @brief A scheduler that runs all tasks inline on current thread.
 */
class inline_scheduler {
public:
  /**
   * @brief The context type for the scheduler.
   */
  class context_type {
  public:
    /**
     * @brief Construct a new context type object, set the thread_local context object to this object.
     */
    context_type() { m_tasks.reserve(128); }

    static void submit(frame_block *ptr) {
      LF_ASSERT(ptr);
      ptr->resume_external<context_type>();
    }

    /**
     * @brief Returns one as this runs all tasks inline.
     */
    static constexpr auto max_threads() noexcept -> std::size_t { return 1; }

    /**
     * @brief Pops a task from the task queue.
     */
    auto task_pop() -> frame_block * {
      if (m_tasks.empty()) {
        return {};
      }
      frame_block *task = m_tasks.back();
      m_tasks.pop_back();
      return task;
    }

    /**
     * @brief Pushes a task to the task queue.
     */
    void task_push(frame_block *task) {
      LF_ASSERT(task);
      m_tasks.push_back(task);
    }

    void stack_push(async_stack *stack) { delete stack; }

    auto stack_pop() -> async_stack * { return new async_stack; }

  private:
    std::vector<frame_block *> m_tasks;
  };

  static_assert(thread_context<context_type>);

private:
  context_type m_context;
};

} // namespace lf

#endif /* C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC */
