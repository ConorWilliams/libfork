#ifndef C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC
#define C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "libfork/core.hpp"
#include "libfork/thread_local.hpp"

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
  class context_type : thread_local_ptr<context_type> {

  public:
    /**
     * @brief The stack type for the scheduler.
     */
    using stack_type = virtual_stack<detail::mebibyte>;
    /**
     * @brief Construct a new context type object, set the thread_local context object to this object.
     */
    context_type() noexcept {
      inline_scheduler::context_type::set(*this);
    }
    /**
     * @brief Get the thread_local context object.
     */
    static auto context() -> context_type & { return context_type::get(); }
    /**
     * @brief Returns one as this runs all tasks inline.
     */
    static constexpr auto max_threads() noexcept -> std::size_t { return 1; }
    /**
     * @brief Get the top stack object.
     */
    auto stack_top() -> stack_type::handle { return stack_type::handle{m_stack.get()}; }
    /**
     * @brief Should never be called, aborts the program.
     */
    static void stack_pop() { LF_ASSERT(false); }
    /**
     * @brief Should never be called, aborts the program.
     */
    static void stack_push([[maybe_unused]] stack_type::handle handle) { LF_ASSERT(false); }
    /**
     * @brief Pops a task from the task queue.
     */
    auto task_pop() -> std::optional<task_handle> {
      if (m_tasks.empty()) {
        return std::nullopt;
      }
      task_handle task = m_tasks.back();
      m_tasks.pop_back();
      return task;
    }
    /**
     * @brief Pushes a task to the task queue.
     */
    void task_push(task_handle task) {
      m_tasks.push_back(task);
    }

  private:
    std::vector<task_handle> m_tasks;
    typename stack_type::unique_ptr_t m_stack = stack_type::make_unique();
  };

  /**
   * @brief Immediately resume the root task.
   */
  static void schedule(stdx::coroutine_handle<> root_task) {
    root_task.resume();
  }

private:
  context_type m_context;
};

static_assert(scheduler<inline_scheduler>);

} // namespace lf

#endif /* C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC */
