#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stack>

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
};

static_assert(context<inline_context>);

}  // namespace lf