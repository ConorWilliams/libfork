// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

#include "libfork/inline.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

task<void, inline_context> test_void(int& count) {
  ++count;
  std::cout << "counter " << count << std::endl;

  co_return;
}

/**
 * What may we want to do when we fork?
 *
 * Design space
 *
 */

TEST_CASE("Basic task manipulation", "[task]") {
  int count = 0;

  auto t = test_void(count);

  inline_context context{};

  context.push(t.m_promise);

  t.m_promise->m_stack = &context;

  context.run();

  REQUIRE(false);
}

// /**
//  * @brief Called at end of coroutine frame.
//  *
//  * Resumes parent task if we are the last child.
//  */
// [[nodiscard]] auto final_suspend() const noexcept {  // NOLINT
//   struct final_awaitable : std::suspend_always {
//     // NOLINTNEXTLINE(readability-function-cognitive-complexity)
//     [[nodiscard]] auto await_suspend(coro_h<promise_type> handle) const noexcept -> coro_h<> {
//       //
//       promise_type const& promise = handle.promise();

//       FORK_LOG("task reaches final suspend");

//       if (!promise.m_parent) {
//         FORK_LOG("task is parentless and returns");
//         return destroy(handle, std::noop_coroutine());
//       }

//       ASSERT_ASSUME(promise.m_stack, "execution context not set");

//       if (promise* parent_handle = promise.m_stack->pop()) {
//         // No-one stole continuation, we are the exclusive owner of parent, just keep rippin!
//         FORK_LOG("fast path, keeps ripping");

//         ASSERT_ASSUME(promise.m_parent, "parent is null -> task is root but, pop() non-null");
//         ASSERT_ASSUME(promise.m_parent->m_this == parent_handle->m_this, "pop() is not parent");
//         ASSERT_ASSUME(parent_handle->m_this, "parent's this handle is null");
//         ASSERT_ASSUME(parent_handle->m_stack == promise.m_stack, "stack changed");

//         return destroy(handle, parent_handle->m_this);  // Destrory the child, resume parent.
//       }

//       FORK_LOG("task's parent was stolen");
//       ASSERT_ASSUME(promise.m_parent, "parent is null");
//       // Register with parent we have completed this child task.
//       auto children_to_join = promise.m_parent->m_join.fetch_sub(1, std::memory_order_release);

//       if (children_to_join == 1) {
//         // Parent has reached join and we are the last child task to complete.
//         // We are the exclusive owner of the parent.
//         // Hence, we should continue parent, therefore we must set the parents context.

//         std::atomic_thread_fence(std::memory_order_acquire);

//         FORK_LOG("task is last child to join and resumes parent");

//         ASSERT_ASSUME(promise.m_parent->m_this, "parent's this handle is null");
//         ASSERT(promise.m_parent->m_stack != promise.m_stack, "parent has same context");

//         promise.m_parent->m_stack = promise.m_stack;

//         return destroy(handle, promise.m_parent->m_this);
//       }
//       // Parent has not reached join or we are not the last child to complete.
//       // We are now out of jobs, yield to executor.
//       FORK_LOG("task is not last to join");
//       return destroy(handle, std::noop_coroutine());
//     }
//   };
//   return final_awaitable{};
// }

// NOLINTEND