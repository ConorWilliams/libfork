#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stack>

#include "libfork/inline.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

/**
 * @file inline.hpp
 *
 * @brief An execution context that runs tasks inline.
 */

namespace lf {
/**
 * @brief An execution context that runs tasks inline.
 */
struct inline_context : private std::stack<promise<inline_context>*> {
  /**
   * @brief A noop as tasks cannot be stolen from an inline context.
   */
  void push(promise<inline_context>* job) noexcept {
    FORK_LOG("inline_context::push()");
    std::stack<promise<inline_context>*>::push(job);
  }

  /**
   * @brief Just returns the parent as tasks cannot be stolen from an inline context.
   */
  auto pop() noexcept -> promise<inline_context>* {
    if (empty()) {
      FORK_LOG("inline_context::pop() called on empty stack");
      return nullptr;
    }
    FORK_LOG("inline_context::pop() called on non-empty stack");
    auto* tmp = std::stack<promise<inline_context>*>::top();
    std::stack<promise<inline_context>*>::pop();
    return tmp;
  }
  /**
   * @brief do the work
   *
   */
  void run() noexcept {
    //
    promise<inline_context>* job = pop();

    if (!job) {
      FORK_LOG("no work to do");
      return;
    }

    while (true) {
      // Store parent as resume MAY destroy jobs promise.
      std::exchange(job, job->m_parent)->m_this.resume();

      if (job == nullptr) {
        FORK_LOG("task is parentless -> root task completed");
        ASSERT_ASSUME(empty(), "should be a root task");
        return;
      }

      // hit a join:

      // Only possible with a stolen coroutine, but knowing it is stolen  alone is not sufficient to
      // know if it is join or final suspend.

      // or a final suspend:

      if (promise<inline_context>* parent = pop()) {
        // No-one stole continuation, we are the exclusive owner of parent, just keep rippin!
        FORK_LOG("fast path, keeps ripping");

        ASSERT_ASSUME(job == parent, "pop() is not parent");
        ASSERT_ASSUME(job->m_this, "parent's this handle is null");
        ASSERT_ASSUME(job->m_stack == this, "stack changed");

        continue;
      }

      FORK_LOG("task's parent was stolen");

      ASSERT_ASSUME(empty(), "stack should be empty");

      // Register with parent we have completed this child task.
      auto children_to_join = job->m_join.fetch_sub(1, std::memory_order_release);

      if (children_to_join == 1) {
        // Parent has reached join and we are the last child task to complete.
        // We are the exclusive owner of the parent.
        // Hence, we should continue parent, therefore we must set the parents context.

        std::atomic_thread_fence(std::memory_order_acquire);

        FORK_LOG("task is last child to join and resumes parent");

        ASSERT_ASSUME(job->m_this, "parent's this handle is null");
        ASSERT(job->m_stack != this, "parent has same context");

        job->m_stack = this;

        continue;
      }
      // We are out of tasks and need to start stealing.
      FORK_LOG("task is not last child to join and steals parent");
    }
  }
};

static_assert(context<inline_context>);

}  // namespace lf