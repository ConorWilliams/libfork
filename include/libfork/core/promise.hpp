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

#include "libfork/macro.hpp"

#include "libfork/core/core.hpp"
#include "libfork/core/coroutine.hpp"
#include "libfork/core/task.hpp"

/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::detail {

// -------------------------------------------------------------------------- //

/**
 * @brief An awaitable type (in a task) that triggers a join.
 */
struct join_t {};

#ifndef NDEBUG
  #define FATAL_IN_DEBUG(expr, message)                                                                      \
    do {                                                                                                     \
      if (!(expr)) {                                                                                         \
        ::lf::detail::noexcept_invoke([] { LF_THROW(std::runtime_error(message)); });                        \
      }                                                                                                      \
    } while (false)
#else
  #define FATAL_IN_DEBUG(expr, message)                                                                      \
    do {                                                                                                     \
    } while (false)
#endif

// Add in a get/set functions that return a reference to the return object.
template <typename Ret>
struct shim_ret_obj : promise_base {

  using ret_ref = std::conditional_t<std::is_void_v<Ret>, int, std::add_lvalue_reference_t<Ret>>;

  template <typename = void>
    requires(!std::is_void_v<Ret>)
  [[nodiscard]] constexpr auto get_return_address_obj() noexcept -> ret_ref {
    return *static_cast<Ret *>(ret_address());
  }

  template <typename = void>
    requires(!std::is_void_v<Ret>)
  [[nodiscard]] constexpr auto set_ret_address(ret_ref obj) noexcept {
    promise_base::set_ret_address(std::addressof(obj));
  }
};

// Specialization for both non-void
template <typename Ret, typename T, tag Tag>
struct mixin_return : shim_ret_obj<Ret> {
  template <typename U>
    requires std::constructible_from<T, U> &&
             (std::is_void_v<Ret> || std::is_assignable_v<std::add_lvalue_reference_t<Ret>, U>)
  void return_value([[maybe_unused]] U &&expr) noexcept(
      std::is_void_v<Ret> || std::is_nothrow_assignable_v<std::add_lvalue_reference_t<Ret>, U>) {
    if constexpr (!std::is_void_v<Ret>) {
      this->get_return_address_obj() = std::forward<U>(expr);
    }
  }
};

// Specialization for void returning tasks.
template <typename Ret, tag Tag>
struct mixin_return<Ret, void, Tag> : shim_ret_obj<Ret> {
  static constexpr void return_void() noexcept {}
};

// Adds a context_type type alias to T.
template <thread_context Context, typename Head>
struct with_context : Head {
  using context_type = Context;
  static auto context() -> Context & { return Context::context(); }
};

template <typename R, thread_context Context, typename Head>
struct shim_with_context : with_context<Context, Head> {
  using return_address_t = R;
};

struct regular_void {};

template <typename Ret, typename T, thread_context Context, tag Tag>
struct promise_type : mixin_return<Ret, T, Tag> {
private:
  template <typename Head, typename... Tail>
  constexpr auto add_context_to_packet(packet<Head, Tail...> &&pack)
      -> packet<with_context<Context, Head>, Tail...> {
    return {pack.ret, {std::move(pack.context)}, std::move(pack.args)};
  }

public:
  using value_type = T;

  [[nodiscard]] static auto operator new(std::size_t const size) -> void * {
    if constexpr (Tag == tag::root) {
      // Use global new.
      return ::operator new(size);
    } else {
      // Use the stack that the thread owns which may not be equal to the parent's stack.
      return Context::context().stack_top()->allocate(size);
    }
  }

  static void operator delete(void *const ptr) noexcept { throw std::runtime_error("Nauty compiler"); }

  static void operator delete(void *const ptr, std::size_t const size) noexcept {
    if constexpr (Tag == tag::root) {
#ifdef __cpp_sized_deallocation
      ::operator delete(ptr, size);
#else
      ::operator delete(ptr);
#endif
    } else {
      // When destroying a task we must be the running on the current threads stack.
      LF_ASSERT(virtual_stack::from_address(ptr) == Context::context().stack_top());
      virtual_stack::from_address(ptr)->deallocate(ptr, size);
    }
  }

  auto get_return_object() -> task<T> {
    return task<T>{stdx::coroutine_handle<promise_type>::from_promise(*this).address()};
  }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  void unhandled_exception() noexcept {

    FATAL_IN_DEBUG(this->debug_count() == 0, "Unhandled exception without a join!");

    if constexpr (Tag == tag::root) {
      LF_LOG("Unhandled exception in root task");
      // Put in our remote root-block.
      this->get_return_address_obj().exception.unhandled_exception();
    } else if (!this->parent().promise().has_parent()) {
      LF_LOG("Unhandled exception in child of root task");
      // Put in parent (root) task's remote root-block.
      // This reinterpret_cast is safe because of the static_asserts in core.hpp.
      reinterpret_cast<exception_packet *>(this->parent().promise().ret_address())
          ->unhandled_exception(); // NOLINT
    } else {
      LF_LOG("Unhandled exception in root's grandchild or further");
      // Put on stack of parent task.
      virtual_stack::from_address(&this->parent().promise())->unhandled_exception();
    }
  }

  auto final_suspend() noexcept {
    struct final_awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> child) const noexcept
          -> stdx::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LF_LOG("Root task at final suspend, releases sem");

          // Finishing a root task implies our stack is empty and should have no exceptions.
          LF_ASSERT(Context::context().stack_top()->empty());

          child.promise().get_return_address_obj().semaphore.release();

          child.destroy();
          return stdx::noop_coroutine();
        }

        LF_LOG("Task reaches final suspend");

        // Must copy onto stack before destroying child.
        stdx::coroutine_handle<promise_base> const parent_h = child.promise().parent();
        // Can no longer touch child.
        child.destroy();

        if constexpr (Tag == tag::call || Tag == tag::invoke) {
          LF_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent_h;
        }

        Context &context = Context::context();

        if (std::optional<task_handle> parent_task_handle = context.task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LF_LOG("Parent not stolen, keeps ripping");
          LF_ASSERT(parent_h.address() == parent_task_handle->address());
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

        LF_LOG("Task's parent was stolen");

        promise_base &parent_cb = parent_h.promise();

        // Register with parent we have completed this child task.
        if (parent_cb.joins().fetch_sub(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent therefore, we must continue parent.

          LF_LOG("Task is last child to join, resumes parent");

          if (parent_cb.has_parent()) {
            // Must take control of stack if we do not already own it.
            auto parent_stack = virtual_stack::from_address(&parent_cb);
            auto thread_stack = context.stack_top();

            if (parent_stack != thread_stack) {
              LF_LOG("Thread takes control of parent's stack");
              LF_ASSUME(thread_stack->empty());
              context.stack_push(parent_stack);
            }
          }

          // Must reset parents control block before resuming parent.
          parent_cb.reset();

          return parent_h;
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LF_LOG("Task is not last to join");

        if (parent_cb.has_parent()) {
          // We are unable to resume the parent, if we were its creator then we should pop a stack
          // from our context as the resuming thread will take ownership of the parent's stack.
          auto parent_stack = virtual_stack::from_address(&parent_cb);
          auto thread_stack = context.stack_top();

          if (parent_stack == thread_stack) {
            LF_LOG("Thread releases control of parent's stack");
            context.stack_pop();
          }
        }

        LF_ASSUME(context.stack_top()->empty());

        return stdx::noop_coroutine();
      }
    };

    FATAL_IN_DEBUG(this->debug_count() == 0, "Fork/Call without a join!");

    LF_ASSERT(this->steals() == 0);            // Fork without join.
    LF_ASSERT(this->joins().load() == k_imax); // Destroyed in invalid state.

    return final_awaitable{};
  }

public:
  template <typename R, typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto
  await_transform(packet<first_arg_t<R, tag::fork, F, This...>, Args...> &&packet) {

    this->debug_inc();

    auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

    stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> parent) noexcept
          -> decltype(child) {
        // In case *this (awaitable) is destructed by stealer after push
        stdx::coroutine_handle stack_child = m_child;

        LF_LOG("Forking, push parent to context");

        Context::context().task_push(task_handle{promise_type::cast_down(parent)});

        return stack_child;
      }

      decltype(child) m_child;
    };

    return awaitable{{}, child};
  }

  template <typename R, typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto
  await_transform(packet<first_arg_t<R, tag::call, F, This...>, Args...> &&packet) {

    this->debug_inc();

    auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

    stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto
      await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept
          -> decltype(child) {
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
  [[nodiscard]] constexpr auto
  await_transform(packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet) {

    FATAL_IN_DEBUG(this->debug_count() == 0, "Invoke within async scope!");

    using value_type_child = typename packet<first_arg_t<void, tag::invoke, F, This...>, Args...>::value_type;

    using wrapped_value_type =
        std::conditional_t<std::is_reference_v<value_type_child>,
                           std::reference_wrapper<std::remove_reference_t<value_type_child>>,
                           value_type_child>;

    using return_type =
        std::conditional_t<std::is_void_v<value_type_child>, regular_void, std::optional<wrapped_value_type>>;

    using packet_type =
        packet<shim_with_context<return_type, Context, first_arg_t<void, tag::invoke, F, This...>>, Args...>;

    using handle_type = typename packet_type::handle_type;

    static_assert(std::same_as<value_type_child, typename packet_type::value_type>,
                  "An async function's value_type must be return_address_t independent!");

    struct awaitable : stdx::suspend_always {

      explicit constexpr awaitable(promise_type *in_self,
                                   packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet)
          : self(in_self),
            m_child(packet_type{m_res, {std::move(in_packet.context)}, std::move(in_packet.args)}.invoke_bind(
                cast_down(stdx::coroutine_handle<promise_type>::from_promise(*self)))) {}

      [[nodiscard]] constexpr auto
      await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept -> handle_type {
        return m_child;
      }

      [[nodiscard]] constexpr auto await_resume() -> value_type_child {

        LF_ASSERT(self->steals() == 0);

        // Propagate exceptions.
        if constexpr (LF_PROPAGATE_EXCEPTIONS) {
          if constexpr (Tag == tag::root) {
            self->get_return_address_obj().exception.rethrow_if_unhandled();
          } else {
            virtual_stack::from_address(self)->rethrow_if_unhandled();
          }
        }

        if constexpr (!std::is_void_v<value_type_child>) {
          LF_ASSERT(m_res.has_value());
          return std::move(*m_res);
        }
      }

      return_type m_res;
      promise_type *self;
      handle_type m_child;
    };

    return awaitable{this, std::move(in_packet)};
  }

  constexpr auto await_transform([[maybe_unused]] join_t join_tag) noexcept {
    struct awaitable {
    private:
      constexpr void take_stack_reset_control() const noexcept {
        // Steals have happened so we cannot currently own this tasks stack.
        LF_ASSUME(self->steals() != 0);

        if constexpr (Tag != tag::root) {

          LF_LOG("Thread takes control of task's stack");

          Context &context = Context::context();

          auto tasks_stack = virtual_stack::from_address(self);
          auto thread_stack = context.stack_top();

          LF_ASSERT(thread_stack != tasks_stack);
          LF_ASSERT(thread_stack->empty());

          context.stack_push(tasks_stack);
        }

        // Some steals have happened, need to reset the control block.
        self->reset();
      }

    public:
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // If no steals then we are the only owner of the parent and we are ready to join.
        if (self->steals() == 0) {
          LF_LOG("Sync ready (no steals)");
          // Therefore no need to reset the control block.
          return true;
        }
        // Currently:            joins() = k_imax - num_joined
        // Hence:       k_imax - joins() = num_joined

        // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
        // better if we see all the decrements to joins() and avoid suspending
        // the coroutine if possible. Cannot fetch_sub() here and write to frame
        // as coroutine must be suspended first.
        auto joined = k_imax - self->joins().load(std::memory_order_acquire);

        if (self->steals() == joined) {
          LF_LOG("Sync is ready");

          take_stack_reset_control();

          return true;
        }

        LF_LOG("Sync not ready");
        return false;
      }

      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> task) noexcept
          -> stdx::coroutine_handle<> {
        // Currently        joins  = k_imax - num_joined
        // We set           joins  = joins() - (k_imax - num_steals)
        //                         = num_steals - num_joined

        // Hence            joined = k_imax - num_joined
        //         k_imax - joined = num_joined

        auto steals = self->steals();
        auto joined = self->joins().fetch_sub(k_imax - steals, std::memory_order_release);

        if (steals == k_imax - joined) {
          // We set joins after all children had completed therefore we can resume the task.

          // Need to acquire to ensure we see all writes by other threads to the result.
          std::atomic_thread_fence(std::memory_order_acquire);

          LF_LOG("Wins join race");

          take_stack_reset_control();

          return task;
        }
        // Someone else is responsible for running this task and we have run out of work.
        LF_LOG("Looses join race");

        // We cannot currently own this stack.

        if constexpr (Tag != tag::root) {
          LF_ASSERT(virtual_stack::from_address(self) != Context::context().stack_top());
        }
        LF_ASSERT(Context::context().stack_top()->empty());

        return stdx::noop_coroutine();
      }

      constexpr void await_resume() const {
        LF_LOG("join resumes");
        // Check we have been reset.
        LF_ASSERT(self->steals() == 0);
        LF_ASSERT(self->joins() == k_imax);

        self->debug_reset();

        if constexpr (Tag != tag::root) {
          LF_ASSERT(virtual_stack::from_address(self) == Context::context().stack_top());
        }

        // Propagate exceptions.
        if constexpr (LF_PROPAGATE_EXCEPTIONS) {
          if constexpr (Tag == tag::root) {
            self->get_return_address_obj().exception.rethrow_if_unhandled();
          } else {
            virtual_stack::from_address(self)->rethrow_if_unhandled();
          }
        }
      }

      promise_type *self;
    };

    return awaitable{this};
  }

private:
  template <typename Promise>
  static auto cast_down(stdx::coroutine_handle<Promise> this_handle) -> stdx::coroutine_handle<promise_base> {

    // Static checks that UB is OK...

    static_assert(alignof(Promise) == alignof(promise_base), "Promise_type must be aligned to U!");

#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_base_of_v<promise_base, Promise>);
#endif

    stdx::coroutine_handle cast_handle =
        stdx::coroutine_handle<promise_base>::from_address(this_handle.address());

    // Run-time check that UB is OK.
    LF_ASSERT(cast_handle.address() == this_handle.address());
    LF_ASSERT(stdx::coroutine_handle<>{cast_handle} == stdx::coroutine_handle<>{this_handle});
    LF_ASSERT(static_cast<void *>(&cast_handle.promise()) == static_cast<void *>(&this_handle.promise()));

    return cast_handle;
  }
};

#undef FATAL_IN_DEBUG

} // namespace lf::detail

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
