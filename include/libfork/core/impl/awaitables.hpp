#ifndef CF3E6AC4_246A_4131_BF7A_FE5CD641A19B
#define CF3E6AC4_246A_4131_BF7A_FE5CD641A19B

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>    // for memory_order_acquire, atomi...
#include <bit>       // for bit_cast
#include <coroutine> // for coroutine_handle, noop_coro...
#include <cstddef>   // for size_t
#include <cstdint>   // for uint16_t
#include <memory>    // for uninitialized_default_const...
#include <span>      // for span
#include <utility>   // for declval

#include "libfork/core/co_alloc.hpp"     // for co_allocable, co_new_t, sta...
#include "libfork/core/ext/context.hpp"  // for full_context
#include "libfork/core/ext/handles.hpp"  // for impl::submit_t, task_handle
#include "libfork/core/ext/list.hpp"     // for unwrap, intrusive_list
#include "libfork/core/ext/tls.hpp"      // for stack, context
#include "libfork/core/impl/frame.hpp"   // for frame
#include "libfork/core/impl/stack.hpp"   // for stack
#include "libfork/core/impl/utility.hpp" // for k_u16_max
#include "libfork/core/invocable.hpp"    // for ignore_t
#include "libfork/core/macro.hpp"        // for LF_ASSERT, LF_LOG, LF_ASSER...
#include "libfork/core/scheduler.hpp"    // for context_switcher

/**
 * @file awaitables.hpp
 *
 * @brief Awaitables (in a `libfork` coroutine) that trigger a switch, fork, call or join.
 */

namespace lf::impl {

// -------------------------------------------------------- //

/**
 * @brief To handle tasks on a WSQ that have been "effectively stolen".
 *
 * If explicit scheduling has occurred then there may be tasks on a workers WSQ that
 * have been "effectively stolen" from another worker. These can be handled in
 * reverse order.
 *
 */
inline LF_FORCEINLINE auto try_self_stealing() noexcept -> std::coroutine_handle<> {
  //
  if (auto *eff_stolen = std::bit_cast<frame *>(tls::context()->pop())) {
    eff_stolen->fetch_add_steal();
    return eff_stolen->self();
  }

  return std::noop_coroutine();
}

// -------------------------------------------------------- //

/**
 * @brief Test if a context switcher has a noexcept initial_suspend().
 */
template <typename A>
concept noexcept_await_ready = context_switcher<A> && requires (std::remove_cvref_t<A> await) {
  { await.await_ready() } noexcept;
};

/**
 * @brief Test if a context switcher has a noexcept await_suspend().
 */
template <typename A>
concept noexcept_await_suspend =
    context_switcher<A> && requires (std::remove_cvref_t<A> await, submit_handle handle) {
      { await.await_suspend(handle) } noexcept;
    };

/**
 * @brief Test if a context switcher has a noexcept await_resume().
 */
template <typename A>
concept noexcept_await_resume = context_switcher<A> && requires (std::remove_cvref_t<A> await) {
  { await.await_resume() } noexcept;
};

/**
 * @brief A wrapper for an ``lf::core::context_switcher`` that
 */
template <context_switcher A>
struct context_switch_awaitable {

  /**
   * @brief Forward to the external awaitable's await_resume().
   */
  auto await_ready() noexcept(noexcept_await_ready<A>) -> bool { return external.await_ready(); }

  /**
   * @brief Reschedule this coro onto `dest`.
   */
  auto
  await_suspend(std::coroutine_handle<> /**/) noexcept(noexcept_await_suspend<A>) -> std::coroutine_handle<> {

    // We currently own the "resumable" handle of this coroutine, if there have been any
    // steals then we do not own the stack this coroutine is on and the resumer should not
    // take the stack otherwise, we should give-it-up and the resumer should take it.

    std::uint16_t steals = std::bit_cast<frame *>(unwrap(&self))->load_steals();

    // Assert the above paragraphs validity.
#ifndef NDEBUG
    if (auto *tmp = std::bit_cast<frame *>(unwrap(&self)); steals == 0) {
      LF_ASSERT(tmp->stacklet() == tls::stack()->top());
    } else {
      LF_ASSERT(tmp->stacklet() != tls::stack()->top());
    }
#endif

    // Schedule this coroutine for execution, cannot touch underlying after this.
    external.await_suspend(&self);

    if (steals == 0) {
      // Dest will take this stack upon resumption hence, we must release it.

      // If this throws (fails to allocate) then the worker must die as
      // it cannot resume a self-stolen task without a stack and we cannot
      // recover the submitted task.
      []() noexcept {
        ignore_t{} = tls::stack()->release();
      }();
    }

    LF_ASSERT(tls::stack()->empty());

    // Eventually dest will fail to pop() the ancestor task that we 'could' pop() here and
    // then treat it as a task that was stolen from it.

    // Now we have a number of tasks on our WSQ which we have "effectively stolen" from dest.
    // All of them will eventually reach a join point.

    // We can pop() the ancestor, mark it stolen and then resume it.

    /**
     * While running the ancestor several things can happen:
     *   We hit a join in the ancestor:
     *      Case Win join:
     *        Take stack, OK to treat tasks on our WSQ as non-stolen.
     *      Case Loose join:
     *        Must treat tasks on our WSQ as stolen.
     *   We loose a join in a descendent of the ancestor:
     *    Ok all task on WSQ must have been stole by other threads and handled as stolen appropriately.
     */

    return try_self_stealing();
  }

  /**
   * @brief Forward to the external awaitable's await_resume().
   */
  auto await_resume() noexcept(noexcept_await_resume<A>) -> decltype(auto) { return external.await_resume(); }

  [[no_unique_address]] A external;            ///< The external awaitable.
  intrusive_list<impl::submit_t *>::node self; ///< The current coroutine's handle.
};

// -------------------------------------------------------- //

/**
 * @brief An awaiter that returns space allocated on the current fibre's stack.
 *
 * This never suspends the coroutine.
 */
template <co_allocable T>
struct alloc_awaitable : std::suspend_never {
  /**
   * @brief Return a handle to the memory.
   */
  [[nodiscard]] auto await_resume() const -> stack_allocated<T> {

    auto *stack = tls::stack();

    LF_ASSERT(stack->top() == self->stacklet()); // Must own the stack.

    T *ptr = static_cast<T *>(stack->allocate(request.count * sizeof(T)));

    // clang-format off

    LF_TRY {
      std::ranges::uninitialized_default_construct_n(ptr, request.count);
    } LF_CATCH_ALL {
      stack->deallocate(ptr);
      LF_RETHROW;
    }

    // clang-format on

    self->reset_stacklet(stack->top());

    return {self, std::span<T>{ptr, request.count}};
  }

  co_new_t<T> request; ///< The requested allocation.
  frame *self;         ///< The current coroutine's frame.
};

// -------------------------------------------------------- //

/**
 * @brief An awaiter that suspends the current coroutine and transfers control to a child task.
 *
 * The parent task is made available for stealing. This is generated by `await_transform` when awaiting on an
 * `lf::impl::quasi_awaitable`.
 */
struct fork_awaitable : std::suspend_always {
  /**
   * @brief Sym-transfer to child, push parent to queue.
   */
  auto await_suspend(std::coroutine_handle<> /*unused*/) const -> std::coroutine_handle<> {
    LF_LOG("Forking, push parent to context");

    // Need a copy (on stack) in case *this is destructed after push.
    std::coroutine_handle child = this->child->self();

    // clang-format off
    
    LF_TRY {
      tls::context()->push(std::bit_cast<task_handle>(parent));
    } LF_CATCH_ALL {
      // If await_suspend throws an exception then: 
      //  - The exception is caught, 
      //  - The coroutine is resumed, 
      //  - The exception is immediately re-thrown.

      // Hence, we need to clean up the child which will never start:
      child.destroy(); 

      LF_RETHROW;
    }

    // clang-format on

    return child;
  }

  frame *child;  ///< The suspended child coroutine's frame.
  frame *parent; ///< The calling coroutine's frame.
};

/**
 * @brief An awaiter that suspends the current coroutine and transfers control to a child task.
 *
 * The parent task is __not__ made available for stealing. This is generated by `await_transform`
 * when awaiting on an `lf::impl::quasi_awaitable`.
 */
struct call_awaitable : std::suspend_always {
  /**
   * @brief Sym-transfer to child.
   */
  auto await_suspend(std::coroutine_handle<> /*unused*/) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Calling");
    return child->self();
  }

  frame *child; ///< The suspended child coroutine's frame.
};

// -------------------------------------------------------------------------------- //

/**
 * @brief An awaiter to synchronize execution of child tasks.
 *
 * This is generated by `await_transform` when awaiting on an `lf::impl::join_type`.
 */
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
  /**
   * @brief Shortcut if children are ready.
   */
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

  /**
   * @brief Mark at join point then yield to scheduler or resume if children are done.
   */
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

    // If explicit scheduling then we may have tasks on our WSQ if we performed a self-steal
    // in a switch awaitable. In this case we can/must do another self-steal.

    return try_self_stealing();
  }

  /**
   * @brief Propagate exceptions.
   */
  void await_resume() const {
    LF_LOG("join resumes");
    // Check we have been reset.
    LF_ASSERT(self->load_steals() == 0);
    LF_ASSERT_NO_ASSUME(self->load_joins(std::memory_order_acquire) == k_u16_max);
    LF_ASSERT(self->stacklet() == tls::stack()->top());

    self->rethrow_if_exception();
  }

  frame *self; ///< The frame of the awaiting coroutine.
};

} // namespace lf::impl

#endif /* CF3E6AC4_246A_4131_BF7A_FE5CD641A19B */
