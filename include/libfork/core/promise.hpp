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

#include "libfork/core/coroutine.hpp"
#include "libfork/core/first_arg.hpp"
#include "libfork/core/promise_base.hpp"
#include "libfork/core/task.hpp"
#include "libfork/macro.hpp"

/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::detail {

// -------------------------------------------------------------------------- //

#ifndef NDEBUG
  #define ASSERT(expr, message)                                 \
    do {                                                        \
      if (!(expr)) {                                            \
        []() noexcept { throw std::runtime_error(message); }(); \
      }                                                         \
    } while (false)
#else
  #define ASSERT(expr, message) \
    do {                        \
    } while (false)
#endif

template <typename T, tag Tag>
struct mixin_return : promise_base {
  template <typename U>
    requires std::constructible_from<T, U> && (Tag == tag::root || Tag == tag::invoke)
  constexpr void return_value(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) {

    LIBFORK_LOG("Root/invoked task returns value");

    using block_t = std::conditional_t<Tag == tag::root, root_block_t<T>, invoke_block_t<T>>;

    auto *block_addr = static_cast<block_t *>(ret_address());
    LIBFORK_ASSERT(!block_addr->result.has_value());
    block_addr->result.emplace(std::forward<U>(expr));
  }

  template <typename U>
    requires std::assignable_from<T &, U> && (Tag == tag::call || Tag == tag::fork)
  void return_value(U &&expr) noexcept(std::is_nothrow_assignable_v<T &, U>) {
    LIBFORK_LOG("Regular task returns a value");
    *static_cast<T *>(ret_address()) = std::forward<U>(expr);
  }
};

template <tag Tag>
struct mixin_return<void, Tag> : promise_base {
  static constexpr void return_void() noexcept {}
};

// Adds a context_type type alias to T.
template <thread_context Context, typename Head>
struct with_context : Head {
  using context_type = Context;
  static auto context() -> Context & { return Context::context(); }
};

template <typename T, thread_context Context, tag Tag>
struct promise_type : mixin_return<T, Tag> {
private:
  template <typename R, typename Head, typename... Tail>
  constexpr auto add_context_to_packet(packet<R, Head, Tail...> pack) -> packet<R, with_context<Context, Head>, Tail...> {
    if constexpr (!std::is_void_v<R>) {
      return {pack.ret, {pack.context}, std::move(pack.args)};
    } else {
      return {{}, {pack.context}, std::move(pack.args)};
    }
  }

  template <class = void>
    requires(Tag == tag::root)
  static auto root_block(void *ret) -> root_block_t<T> & {
    return *static_cast<root_block_t<T> *>(ret);
  }

public:
  using value_type = T;
  using stack_type = typename Context::stack_type;

  [[nodiscard]] static auto operator new(std::size_t const size) -> void * {
    if constexpr (Tag == tag::root) {
      // Use global new.
      return ::operator new(size);
    } else {
      // Use the stack that the thread owns which may not be equal to the parent's stack.
      return Context::context().stack_top()->allocate(size);
    }
  }

  static auto operator delete(void *const ptr, std::size_t const size) noexcept -> void {
    if constexpr (Tag == tag::root) {
#ifdef __cpp_sized_deallocation
      ::operator delete(ptr, size);
#else
      ::operator delete(ptr);
#endif
    } else {
      // When destroying a task we must be the running on the current threads stack.
      LIBFORK_ASSERT(stack_type::from_address(ptr) == Context::context().stack_top());
      stack_type::from_address(ptr)->deallocate(ptr, size);
    }
  }

  auto get_return_object() -> task<T> {
    return task<T>{stdexp::coroutine_handle<promise_type>::from_promise(*this).address()};
  }

  static auto initial_suspend() -> stdexp::suspend_always { return {}; }

  void unhandled_exception() noexcept {

    ASSERT(this->debug_count() == 0, "Unhandled exception without a join!");

    if constexpr (Tag == tag::root) {
      LIBFORK_LOG("Unhandled exception in root task");
      // Put in our remote root-block.
      root_block(this->ret_address()).exception.unhandled_exception();
    } else if (!this->parent().promise().has_parent()) {
      LIBFORK_LOG("Unhandled exception in child of root task");
      // Put in parent (root) task's remote root-block.
      // This reinterpret_cast is safe because of the static_asserts in promise_base.hpp.
      reinterpret_cast<exception_packet *>(this->parent().promise().ret_address())->unhandled_exception(); // NOLINT
    } else {
      LIBFORK_LOG("Unhandled exception in root's grandchild or further");
      // Put on stack of parent task.
      stack_type::from_address(&this->parent().promise())->unhandled_exception();
    }
  }

  auto final_suspend() noexcept {
    struct final_awaitable : stdexp::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdexp::coroutine_handle<promise_type> child) const noexcept -> stdexp::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LIBFORK_LOG("Root task at final suspend, releases sem");

          // Finishing a root task implies our stack is empty and should have no exceptions.
          LIBFORK_ASSERT(Context::context().stack_top()->empty());

          root_block(child.promise().ret_address()).semaphore.release();
          child.destroy();
          return stdexp::noop_coroutine();
        }

        LIBFORK_LOG("Task reaches final suspend");

        // Must copy onto stack before destroying child.
        stdexp::coroutine_handle<promise_base> const parent_h = child.promise().parent();
        // Can no longer touch child.
        child.destroy();

        if constexpr (Tag == tag::call || Tag == tag::invoke) {
          LIBFORK_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent_h;
        }

        Context &context = Context::context();

        if (std::optional<task_handle> parent_task_handle = context.task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LIBFORK_LOG("Parent not stolen, keeps ripping");
          LIBFORK_ASSERT(parent_h.address() == parent_task_handle->address());
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occurred so we do not need to call reset().;
          return parent_h;
        }

        // We are either: the thread that created the parent or a thread that completed a forked task.

        // Note: emptying stack implies finished a stolen task or finished a task forked from root.

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

        promise_base &parent_cb = parent_h.promise();

        // Register with parent we have completed this child task.
        if (parent_cb.joins().fetch_sub(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent therefore, we must continue parent.

          LIBFORK_LOG("Task is last child to join, resumes parent");

          if (parent_cb.has_parent()) {
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

        if (parent_cb.has_parent()) {
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

    ASSERT(this->debug_count() == 0, "Fork/Call without a join!");

    LIBFORK_ASSERT(this->steals() == 0);            // Fork without join.
    LIBFORK_ASSERT(this->joins().load() == k_imax); // Destroyed in invalid state.

    return final_awaitable{};
  }

private:
  template <typename R, typename Head, typename... Args>
    requires((std::is_void_v<R> && Head::tag_value == tag::invoke) || std::is_same_v<R, typename invoker<Head, Args...>::promise_type::value_type>)
  constexpr auto invoke(packet<R, Head, Args...> packet) {

    LIBFORK_LOG("Spawning a task");

    auto unwrap = [&](Args &&...args) {
      return invoker<Head, Args...>::invoke(packet.context, std::forward<Args>(args)...);
    };

    stdexp::coroutine_handle child = std::apply(unwrap, std::move(packet.args));

    child.promise().set_parent(cast_down(stdexp::coroutine_handle<promise_type>::from_promise(*this)));

    if constexpr (!std::is_void_v<R>) {
      child.promise().set_ret_address(std::addressof(packet.ret));
    }

    return child;
  }

public:
  template <typename R, typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<R, first_arg<tag::fork, F, This...>, Args...> packet) {

    this->debug_inc();

    stdexp::coroutine_handle child = invoke(add_context_to_packet(std::move(packet)));

    struct awaitable : stdexp::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdexp::coroutine_handle<promise_type> parent) noexcept -> decltype(child) {
        // In case *this (awaitable) is destructed by stealer after push
        stdexp::coroutine_handle stack_child = m_child;

        LIBFORK_LOG("Forking, push parent to context");

        Context::context().task_push(task_handle{promise_type::cast_down(parent)});

        return stack_child;
      }

      decltype(child) m_child;
    };

    return awaitable{{}, child};
  }

  template <typename R, typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<R, first_arg<tag::call, F, This...>, Args...> packet) {

    this->debug_inc();

    stdexp::coroutine_handle child = invoke(add_context_to_packet(std::move(packet)));

    struct awaitable : stdexp::suspend_always {
      [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdexp::coroutine_handle<promise_type> parent) noexcept -> decltype(child) {
        return m_child;
      }
      decltype(child) m_child;
    };

    return awaitable{{}, child};
  }

  /**
   * @brief An invoke should never occur within an async scope as the exceptions will get muddled
   */
  template <typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<void, first_arg<tag::invoke, F, This...>, Args...> packet) {

    ASSERT(this->debug_count() == 0, "Invoke within async scope!");

    LIBFORK_ASSERT(this->steals() == 0);

    stdexp::coroutine_handle child = invoke(add_context_to_packet(std::move(packet)));

    using child_value_type = typename std::decay_t<decltype(child.promise())>::value_type;

    struct awaitable : stdexp::suspend_always {

      [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdexp::coroutine_handle<promise_type> parent) noexcept -> decltype(child) {
        if constexpr (!std::is_void_v<child_value_type>) {
          m_child.promise().set_ret_address(std::addressof(m_res));
        }
        return m_child;
      }

      [[nodiscard]] constexpr auto await_resume() -> child_value_type {

        LIBFORK_ASSERT(base->steals() == 0);

        // Propagate exceptions.
        if constexpr (LIBFORK_PROPAGATE_EXCEPTIONS) {
          if constexpr (Tag == tag::root) {
            root_block(base->ret_address()).exception.rethrow_if_unhandled();
          } else {
            stack_type::from_address(base)->rethrow_if_unhandled();
          }
        }
        if constexpr (!std::is_void_v<child_value_type>) {
          LIBFORK_ASSERT(m_res.result.has_value());
          return std::move(*m_res.result);
        }
      }

      promise_base *base;
      decltype(child) m_child;
      invoke_block_t<child_value_type> m_res;
    };

    return awaitable{{}, this, child, {}};
  }

  constexpr auto await_transform([[maybe_unused]] join_t join_tag) noexcept {
    struct awaitable {
    private:
      constexpr void take_stack_reset_control() const noexcept {
        // Steals have happened so we cannot currently own this tasks stack.
        LIBFORK_ASSUME(base->steals() != 0);

        if constexpr (Tag != tag::root) {

          LIBFORK_LOG("Thread takes control of task's stack");

          Context &context = Context::context();

          auto tasks_stack = stack_type::from_address(base);
          auto thread_stack = context.stack_top();

          LIBFORK_ASSERT(thread_stack != tasks_stack);
          LIBFORK_ASSERT(thread_stack->empty());

          context.stack_push(tasks_stack);
        }

        // Some steals have happened, need to reset the control block.
        base->reset();
      }

    public:
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // If no steals then we are the only owner of the parent and we are ready to join.
        if (base->steals() == 0) {
          LIBFORK_LOG("Sync ready (no steals)");
          // Therefore no need to reset the control block.
          return true;
        }
        // Currently:            joins() = k_imax - num_joined
        // Hence:       k_imax - joins() = num_joined

        // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
        // better if we see all the decrements to joins() and avoid suspending
        // the coroutine if possible.
        auto joined = k_imax - base->joins().load(std::memory_order_acquire);

        if (base->steals() == joined) {
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

        auto steals = base->steals();
        auto joined = base->joins().fetch_sub(k_imax - steals, std::memory_order_release);

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

        if constexpr (Tag != tag::root) {
          LIBFORK_ASSERT(stack_type::from_address(base) != Context::context().stack_top());
        }
        LIBFORK_ASSERT(Context::context().stack_top()->empty());

        return stdexp::noop_coroutine();
      }

      constexpr void await_resume() const {
        LIBFORK_LOG("join resumes");
        // Check we have been reset.
        LIBFORK_ASSERT(base->steals() == 0);
        LIBFORK_ASSERT(base->joins() == k_imax);

        base->debug_reset();

        if constexpr (Tag != tag::root) {
          LIBFORK_ASSERT(stack_type::from_address(base) == Context::context().stack_top());
        }

        // Propagate exceptions.
        if constexpr (LIBFORK_PROPAGATE_EXCEPTIONS) {
          if constexpr (Tag == tag::root) {
            root_block(base->ret_address()).exception.rethrow_if_unhandled();
          } else {
            stack_type::from_address(base)->rethrow_if_unhandled();
          }
        }
      }

      promise_base *base;
    };

    return awaitable{this};
  }

private:
  template <typename Promise>
  static auto cast_down(stdexp::coroutine_handle<Promise> this_handle) -> stdexp::coroutine_handle<promise_base> {

    // Static checks that UB is OK.

    static_assert(alignof(Promise) == alignof(promise_base), "Promise_type must be aligned to U!");

#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_base_of_v<promise_base, Promise>);
#endif

    stdexp::coroutine_handle cast_handle = stdexp::coroutine_handle<promise_base>::from_address(this_handle.address());

    // Run-time check that UB is OK.
    LIBFORK_ASSERT(cast_handle.address() == this_handle.address());
    LIBFORK_ASSERT(stdexp::coroutine_handle<>{cast_handle} == stdexp::coroutine_handle<>{this_handle});
    LIBFORK_ASSERT(static_cast<void *>(&cast_handle.promise()) == static_cast<void *>(&this_handle.promise()));

    return cast_handle;
  }
};

#undef ASSERT

} // namespace lf::detail

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
