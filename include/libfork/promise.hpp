#ifndef FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0
#define FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

#include "libfork/coroutine.hpp"
#include "libfork/macro.hpp"
#include "libfork/promise_base.hpp"
#include "libfork/thread_local.hpp"

/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf {

template <typename T, thread_context Context>
class task;

namespace detail {

// ------------------- Tag types ------------------- //

// NOLINTBEGIN

/**
 * @brief An awaitable type (in a task) that triggers a join.
 */
struct join_t {};

template <typename R>
struct return_t {
  R &ret;
};

template <>
struct return_t<void> {};

/**
 * @brief An awaitable type (in a task) that triggers a fork.
 */
template <typename R, typename F, typename... Args>
struct [[nodiscard]] fork_packet : return_t<R> {
  [[no_unique_address]] std::tuple<Args &&...> args;
};

/**
 * @brief An awaitable type (in a task) that triggers a call.
 */
template <typename R, typename F, typename... Args>
struct [[nodiscard]] call_packet : return_t<R> {
  [[no_unique_address]] std::tuple<Args &&...> args;
};

// NOLINTEND

// ------------------- Main promise type ------------------- //

template <typename T, thread_context Context, tag Tag>
struct promise_type : promise_base<T> {

  using stack_type = typename Context::stack_type;

  constexpr promise_type() noexcept : promise_base<T>{control_block_t{Tag{}}} {}

  [[nodiscard]] static auto operator new(std::size_t const size) -> void * {
    if constexpr (std::same_as<Tag, root_t>) {
      // Use gloabal new.
      return ::operator new(size);
    } else {
      // Use the stack that the thread owns which may not be equal to the parent's stack.
      return Context::context().stack_top()->allocate(size);
    }
  }

  static auto operator delete(void *const ptr, std::size_t const size) noexcept -> void {
    if constexpr (std::same_as<Tag, root_t>) {
#ifdef __cpp_sized_deallocation
      ::operator delete[](ptr, size);
#else
      ::operator delete[](ptr);
#endif
    } else {
      // When destroying a task we must be the running on the current threads stack.
      LIBFORK_ASSERT(stack_type::from_address(ptr) == Context::context().stack_top());
      stack_type::from_address(ptr)->deallocate(ptr, size);
    }
  }

  auto get_return_object() -> task<T, Context> {
    return task<T, Context>{cast_handle<promise_base<T>>(stdexp::coroutine_handle<promise_type>::from_promise(*this))};
  }

  static auto initial_suspend() -> stdexp::suspend_always { return {}; }

  void unhandled_exception() noexcept {
    if constexpr (std::same_as<Tag, root_t>) {
      LIBFORK_LOG("Unhandled exception in root task");
      // Put in our remote root-block.
      this->control_block.root().unhandled_exception();
    } else if constexpr (std::same_as<Tag, call_from_root_t> || std::same_as<Tag, fork_from_root_t>) {
      LIBFORK_LOG("Unhandled exception in root child task");
      // Put in parent (root) task's remote root-block.
      this->control_block.parent().promise().root().unhandled_exception();
    } else {
      LIBFORK_LOG("Unhandled exception in root grandchild or further");
      // Put on stack of parent task.
      stack_type::from_address(&this->control_block.parent().promise())->unhandled_exception();
    }
  }

  auto final_suspend() noexcept {
    struct final_awaitable : stdexp::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdexp::coroutine_handle<promise_type> child) const noexcept -> stdexp::coroutine_handle<> {

        if constexpr (std::same_as<Tag, root_t>) {
          LIBFORK_LOG("Root task at final suspend, releases sem");

          // Finishing a root task implies our stack is empty and should have no exceptions.
          LIBFORK_ASSERT(Context::context().stack_top()->empty());

          child.promise().control_block.root().release();
          child.destroy();
          return stdexp::noop_coroutine();
        }

        LIBFORK_LOG("Task reaches final suspend");

        // Must copy onto stack before destroying child.
        stdexp::coroutine_handle<control_block_t> const parent_h = child.promise().control_block.parent();
        // Can no longer touch child.
        child.destroy();

        if constexpr (std::same_as<Tag, call_t> || std::same_as<Tag, call_from_root_t>) {
          LIBFORK_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent_h;
        }

        Context &context = Context::context();

        if (std::optional<task_handle> parent_task_handle = context.task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep rippin!
          LIBFORK_LOG("Parent not stolen, keeps ripping");
          LIBFORK_ASSERT(parent_h == *parent_task_handle);
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occured so we do not need to call reset().;
          return parent_h;
        }

        // We are either: the thread that created the parent or a thread that completed a forked task.

        // Note: emptying stack implies finised a stolen task or finished a task forked from root.

        // Cases:
        // 1. We are fork_from_root_t
        //    - Every task forked from root is the the first task on a stack -> stack is empty now.
        //      Parent (root) is not on a stack so do not need to take/release control
        // 2. We are fork_t
        //    - Stack is empty -> we cannot be the thread that created the parent as it would be on our stack.
        //    - Stack is non-empty -> we must be the creator of the parent

        // If we created the parent then our current stack is non empty (unless the parent is a root task).
        // If we did not create the parent then we just cleared our current stack and it is now empty.

        LIBFORK_LOG("Task's parent was stolen");

        control_block_t &parent_cb = parent_h.promise();

        // Register with parent we have completed this child task.
        if (parent_cb.joins().fetch_sub(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent therefore, we must continue parent.

          LIBFORK_LOG("Task is last child to join, resumes parent");

          if constexpr (std::same_as<Tag, fork_t>) {
            // Must take control of stack if we do not already own it.
            auto parent_stack = stack_type::from_address(&parent_cb);
            auto thread_stack = context.stack_top();

            if (parent_stack != thread_stack) {
              LIBFORK_LOG("Thread takes control of parent's stack");
              LIBFORK_ASSUME(thread_stack->empty());
              context.stack_push(parent_stack);
            }
          }

          // Must reset parents control block before resuming parent.
          parent_cb.reset();

          return parent_h;
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LIBFORK_LOG("Task is not last to join");

        if constexpr (std::same_as<Tag, fork_t>) {
          // We are unable to resume the parent, if we were its creator then we should pop a stack
          // from our context as the resuming thread will take ownership of the parent's stack.
          auto parent_stack = stack_type::from_address(&parent_cb);
          auto thread_stack = context.stack_top();

          if (parent_stack == thread_stack) {
            LIBFORK_LOG("Thread releases control of parent's stack");
            context.stack_pop();
          }
        }

        LIBFORK_ASSUME(context.stack_top()->empty());

        return stdexp::noop_coroutine();
      }
    };

    LIBFORK_ASSERT(this->control_block.steals() == 0);            // Fork without join.
    LIBFORK_ASSERT(this->control_block.joins().load() == k_imax); // Destroyed in invalid state.

    return final_awaitable{};
  }

private:
  template <typename TagWith, template <typename...> typename Packet, typename R, typename F, typename... Args>
    requires std::is_invocable_r_v<task<R, Context>, F, TagWith, Args...>
  constexpr auto invoke_with(Packet<R, F, Args...> packet) -> stdexp::coroutine_handle<promise_base<R>> {

    LIBFORK_LOG("Spawning a task");

    auto inject = [&](Args &&...args) {
      return std::invoke(F{}, TagWith{}, std::forward<Args>(args)...);
    };

    stdexp::coroutine_handle<promise_base<R>> child = std::apply(inject, std::move(packet.args));

    auto this_handle = stdexp::coroutine_handle<promise_type>::from_promise(*this);

    child.promise().control_block.set(cast_handle<control_block_t>(this_handle));

    if constexpr (!std::is_void_v<R>) {
      child.promise().set_return_address(packet.ret);
    }

    return child;
  }

  using call_child_tag = std::conditional_t<std::same_as<Tag, root_t>, call_from_root_t, call_t>;

  using fork_child_tag = std::conditional_t<std::same_as<Tag, root_t>, fork_from_root_t, fork_t>;

public:
  template <typename R, typename F, typename... Args, typename Magic = magic<fork_child_tag, wrap_fn<F>>>
    requires std::is_invocable_r_v<task<R, Context>, F, Magic, Args...>
  [[nodiscard]] constexpr auto await_transform(fork_packet<R, F, Args...> packet) {
    //
    struct awaitable : stdexp::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdexp::coroutine_handle<promise_type> parent) noexcept -> stdexp::coroutine_handle<> {
        // In case *this (awaitable) is destructed by stealer after push
        stdexp::coroutine_handle<> stack_child = m_child;

        LIBFORK_LOG("Forking, push parent to context");

        Context::context().task_push(task_handle{cast_handle<control_block_t>(parent)});

        return stack_child;
      }

      stdexp::coroutine_handle<promise_base<R>> m_child;
    };

    return awaitable{{}, invoke_with<Magic>(std::move(packet))};
  }

  template <typename R, typename F, typename... Args, typename Magic = magic<call_child_tag, wrap_fn<F>>>
    requires std::is_invocable_r_v<task<R, Context>, F, Magic, Args...>
  [[nodiscard]] constexpr auto await_transform(call_packet<R, F, Args...> packet) {
    //
    struct awaitable : stdexp::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdexp::coroutine_handle<promise_type>) noexcept -> stdexp::coroutine_handle<promise_base<R>> {
        return m_child;
      }

      stdexp::coroutine_handle<promise_base<R>> m_child;
    };

    return awaitable{{}, invoke_with<Magic>(std::move(packet))};
  }

  constexpr auto await_transform(join_t) noexcept {
    struct awaitable {
    private:
      constexpr void take_stack_reset_control() const noexcept {
        // Steals have happend so we cannot currently own this tasks stack.
        LIBFORK_ASSUME(control_block->steals() != 0);

        if constexpr (!std::same_as<Tag, root_t>) {

          LIBFORK_LOG("Thread takes control of task's stack");

          Context &context = Context::context();

          auto tasks_stack = stack_type::from_address(control_block);
          auto thread_stack = context.stack_top();

          LIBFORK_ASSERT(thread_stack != tasks_stack);
          LIBFORK_ASSERT(thread_stack->empty());

          context.stack_push(tasks_stack);
        }

        // Some steals have happend, need to reset the control block.
        control_block->reset();
      }

    public:
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // If no steals then we are the only owner of the parent and we are ready to join.
        if (control_block->steals() == 0) {
          LIBFORK_LOG("Sync ready (no steals)");
          // Therefore no need to reset the control block.
          return true;
        }
        // Currently:            joins() = k_imax - num_joined
        // Hence:       k_imax - joins() = num_joined

        // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
        // better if we see all the decrements to joins() and avoid suspending
        // the coroutine if possible.
        auto joined = k_imax - control_block->joins().load(std::memory_order_acquire);

        if (control_block->steals() == joined) {
          LIBFORK_LOG("Sync is ready");

          take_stack_reset_control();

          return true;
        }

        LIBFORK_LOG("Sync not ready");
        return false;
      }

      [[nodiscard]] constexpr auto await_suspend(stdexp::coroutine_handle<promise_type> task) noexcept -> stdexp::coroutine_handle<> {
        // Currently        joins  = k_imax - num_joined
        // We set           joins  = joins() - (k_imax - num_steals)
        //                         = num_steals - num_joined

        // Hence            joined = k_imax - num_joined
        //         k_imax - joined = num_joined

        //  Consider race condition on write to m_context.

        auto steals = control_block->steals();
        auto joined = control_block->joins().fetch_sub(k_imax - steals, std::memory_order_release);

        if (steals == k_imax - joined) {
          // We set n after all children had completed therefore we can resume the task.

          // Need to acquire to ensure we see all writes by other threads to the result.
          std::atomic_thread_fence(std::memory_order_acquire);

          LIBFORK_LOG("Wins join race");

          take_stack_reset_control();

          return task;
        }
        // Someone else is responsible for running this task and we have run out of work.
        LIBFORK_LOG("Looses join race");

        // We cannot currently own this stack.

        if constexpr (!std::same_as<Tag, root_t>) {
          LIBFORK_ASSERT(stack_type::from_address(control_block) != Context::context().stack_top());
        }
        LIBFORK_ASSERT(Context::context().stack_top()->empty());

        return stdexp::noop_coroutine();
      }

      constexpr void await_resume() const {
        // Check we have been reset.
        LIBFORK_ASSERT(control_block->steals() == 0);
        LIBFORK_ASSERT(control_block->joins() == k_imax);

        if constexpr (!std::same_as<Tag, root_t>) {
          LIBFORK_ASSERT(stack_type::from_address(control_block) == Context::context().stack_top());
        }

        // Propagate exceptions.
        if constexpr (LIBFORK_PROPAGATE_EXCEPTIONS) {
          if constexpr (std::same_as<Tag, root_t>) {
            control_block->root().rethrow_if_unhandled();
          } else {
            stack_type::from_address(control_block)->rethrow_if_unhandled();
          }
        }
      }

      control_block_t *control_block;
    };

    return awaitable{&this->control_block};
  }

private:
  template <typename U>
  static auto cast_handle(stdexp::coroutine_handle<promise_type> this_handle) -> stdexp::coroutine_handle<U> {

    static_assert(alignof(promise_type) == alignof(U), "Promise_type must be aligned to U!");

    stdexp::coroutine_handle cast_handle = stdexp::coroutine_handle<U>::from_address(this_handle.address());

    // Runt-time check that UB is OK.
    LIBFORK_ASSERT(cast_handle.address() == this_handle.address());
    LIBFORK_ASSERT(stdexp::coroutine_handle<>{cast_handle} == stdexp::coroutine_handle<>{this_handle});
    LIBFORK_ASSERT(static_cast<void *>(&cast_handle.promise()) == static_cast<void *>(&this_handle.promise()));

    return cast_handle;
  }
};

} // namespace detail

} // namespace lf

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
