#ifndef CF3E6AC4_246A_4131_BF7A_FE5CD641A19B
#define CF3E6AC4_246A_4131_BF7A_FE5CD641A19B

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>

#include <span>
#include <type_traits>

#include "libfork/core/invocable.hpp"
#include "libfork/core/macro.hpp"

#include "libfork/core/ext/handles.hpp"
#include "libfork/core/ext/tls.hpp"

#include "libfork/core/impl/frame.hpp"
#include "libfork/core/impl/utility.hpp"

/**
 * @file awaitables.hpp
 *
 * @brief Awaitables (in a `libfork` coroutine) that trigger a switch, fork, call or join.
 */

namespace lf::impl {

// -------------------------------------------------------- //

struct switch_awaitable : std::suspend_always {

  auto await_ready() const noexcept { return tls::context() == dest; }

  auto await_suspend(std::coroutine_handle<>) noexcept -> std::coroutine_handle<> {

    // Schedule this coro for execution on Dest.
    dest->submit(&self);

    // Dest will take this stack upon resumption hence, we must release it.
    ignore_t{} = tls::stack()->release();

    // Eventually dest will fail to pop() the ancestor task that we 'could' pop() here and
    // then treat it as a task that was stolen from it.

    // Now we have a number of tasks on our WSQ which we have "effectively stolen" from dest.
    // All of them will eventually reach a join point.

    // We can pop() the ancestor, mark it stolen and then resume it.

    /**
     * While running the ancestor several things can happen:
     *  - We hit a join in the ancestor:
     *    - Case Win join:
     *      Take stack, OK to treat tasks on our WSQ as non-stolen.
     *    - Case Loose join:
     *      Must treat tasks on our WSQ as stolen.
     * - We loose a join in a descendent of the ancestor:
     *   Ok all task on WSQ must have been stole by other threads and handled as stolen appropriately.
     */

    if (auto *eff_stolen = std::bit_cast<frame *>(tls::context()->pop())) {
      eff_stolen->fetch_add_steal();
      return eff_stolen->self();
    }
    return std::noop_coroutine();
  }

  intrusive_list<submit_handle>::node self;
  context *dest;
};

// -------------------------------------------------------- //

template <typename T, std::size_t E>
struct alloc_awaitable : std::suspend_never, std::span<T, E> {
  [[nodiscard]] auto await_resume() const noexcept -> std::conditional_t<E == 1, T *, std::span<T, E>> {
    if constexpr (E == 1) {
      return this->data();
    } else {
      return *this;
    }
  }
};

// -------------------------------------------------------- //

struct fork_awaitable : std::suspend_always {

  auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Forking, push parent to context");
    // Need a copy (on stack) in case *this is destructed after push.
    std::coroutine_handle child = this->child->self();
    tls::context()->push(std::bit_cast<task_handle>(parent));
    return child;
  }

  frame *child;
  frame *parent;
};

struct call_awaitable : std::suspend_always {

  auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Calling");
    return child->self();
  }

  frame *child;
};

// -------------------------------------------------------------------------------- //

struct join_awaitable {
 private:
  void take_stack_reset_frame() const noexcept {
    // Steals have happened so we cannot currently own this tasks stack.
    LF_ASSERT(self->load_steals() != 0);
    LF_ASSERT(tls::stack()->empty());
    *tls::stack() = stack{self->stacklet()};
    // Some steals have happened, need to reset the control block.
    self->reset();
  }

 public:
  auto await_ready() const noexcept -> bool {
    // If no steals then we are the only owner of the parent and we are ready to join.
    if (self->load_steals() == 0) {
      LF_LOG("Sync ready (no steals)");
      // Therefore no need to reset the control block.
      return true;
    }
    // Currently:            joins() = k_u16_max - num_joined
    // Hence:       k_u16_max - joins() = num_joined

    // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
    // better if we see all the decrements to joins() and avoid suspending
    // the coroutine if possible. Cannot fetch_sub() here and write to frame
    // as coroutine must be suspended first.
    auto joined = k_u16_max - self->load_joins(std::memory_order_acquire);

    if (self->load_steals() == joined) {
      LF_LOG("Sync is ready");
      take_stack_reset_frame();
      return true;
    }

    LF_LOG("Sync not ready");
    return false;
  }

  auto await_suspend(std::coroutine_handle<> task) const noexcept -> std::coroutine_handle<> {
    // Currently        joins  = k_u16_max  - num_joined
    // We set           joins  = joins()    - (k_u16_max - num_steals)
    //                         = num_steals - num_joined

    // Hence               joined = k_u16_max - num_joined
    //         k_u16_max - joined = num_joined

    auto steals = self->load_steals();
    auto joined = self->fetch_sub_joins(k_u16_max - steals, std::memory_order_release);

    if (steals == k_u16_max - joined) {
      // We set joins after all children had completed therefore we can resume the task.
      // Need to acquire to ensure we see all writes by other threads to the result.
      std::atomic_thread_fence(std::memory_order_acquire);
      LF_LOG("Wins join race");
      take_stack_reset_frame();
      return task;
    }
    LF_LOG("Looses join race");

    // Someone else is responsible for running this task.
    // We cannot touch *this or deference self as someone may have resumed already!
    // We cannot currently own this stack (checking would violate above).

    // If no explicit scheduling then we must have an empty WSQ as we stole this task.

    if (auto *eff_stolen = std::bit_cast<frame *>(tls::context()->pop())) {
      eff_stolen->fetch_add_steal();
      return eff_stolen->self();
    }
    return std::noop_coroutine();
  }

  void await_resume() const noexcept {
    LF_LOG("join resumes");
    // Check we have been reset.
    LF_ASSERT(self->load_steals() == 0);
    LF_ASSERT_NO_ASSUME(self->load_joins(std::memory_order_acquire) == k_u16_max);
    LF_ASSERT(self->stacklet() == tls::stack()->top());
  }

  frame *self;
};

} // namespace lf::impl

#endif /* CF3E6AC4_246A_4131_BF7A_FE5CD641A19B */
