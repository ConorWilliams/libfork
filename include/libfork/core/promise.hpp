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
#include "libfork/utility.hpp"

#include "libfork/core/coroutine.hpp"
#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"

/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::detail {

// -------------------------------------------------------------------------- //

// TODO: Cleanup below

// /**
//  * @brief Disable rvalue references for T&& template types if an async function
//  * is forked.
//  *
//  * This is to prevent the user from accidentally passing a temporary object to
//  * an async function that will then destructed in the parent task before the
//  * child task returns.
//  */
// template <typename T, typename Self>
// concept protect_forwarding_tparam = first_arg<Self> && !std::is_rvalue_reference_v<T> &&
//                                     (tag_of<Self> != tag::fork || std::is_reference_v<T>);

/**
 * @brief An awaitable type (in a task) that triggers a join.
 */
struct join_t {};

#ifndef NDEBUG
  #define FATAL_IN_DEBUG(expr, message)                                                                                     \
    do {                                                                                                                    \
      if (!(expr)) {                                                                                                        \
        ::lf::detail::noexcept_invoke([] { LF_THROW(std::runtime_error(message)); });                                       \
      }                                                                                                                     \
    } while (false)
#else
  #define FATAL_IN_DEBUG(expr, message)                                                                                     \
    do {                                                                                                                    \
    } while (false)
#endif

template <tag Tag>
using allocator = std::conditional_t<Tag == tag::root, promise_alloc_heap, promise_alloc_stack>;

template <typename R, typename T, thread_context Context, tag Tag>
struct promise_type : allocator<Tag>, promise_result<R, T> {

  static_assert(Tag == tag::fork || Tag == tag::call || Tag == tag::root);
  static_assert(Tag != tag::root || is_root_result_v<R>);

  using handle_t = stdx::coroutine_handle<promise_type>;

  template <first_arg Head, typename... Tail>
  constexpr promise_type(Head const &head, [[maybe_unused]] Tail &&...tail) noexcept
    requires std::constructible_from<promise_result<R, T>, R *>
      : allocator<Tag>{handle_t::from_promise(*this)},
        promise_result<R, T>{head.address()} {}

  constexpr promise_type() noexcept : allocator<Tag>{handle_t::from_promise(*this)} {}

  static auto get_return_object() noexcept -> task_construct_key { return {}; }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  void unhandled_exception() noexcept { LF_RETHROW; }

  auto final_suspend() noexcept {
    struct final_awaitable : stdx::suspend_always {
      constexpr auto await_suspend(stdx::coroutine_handle<promise_type> child) const noexcept -> stdx::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LF_LOG("Root task at final suspend, releases semaphore");
          // Finishing a root task implies our stack is empty and should have no exceptions.
          child.promise().address()->semaphore.release();
          child.destroy();
          return stdx::noop_coroutine();
        }

        // Completing a non-root task means we currently own the async_stack this child is on

        FATAL_IN_DEBUG(tls::asp->debug_count() == 0, "Fork/Call without a join!");

        LF_ASSERT(tls::asp->steals() == 0);                                      // Fork without join.
        LF_ASSERT(tls::asp->load_joins(std::memory_order_acquire) == k_u16_max); // Destroyed in invalid state.

        LF_LOG("Task reaches final suspend");

        auto [parent, parent_on_asp] = frame_block::pop_asp();

        if constexpr (Tag == tag::call || Tag == tag::invoke) {
          LF_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent->get_coro();
        }

        Context *context = tls::ctx<Context>;

        LF_ASSERT(context);

        if (task_ptr parent_task = context->task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LF_LOG("Parent not stolen, keeps ripping");
          LF_ASSERT(parent_task.m_stolen == parent);
          LF_ASSERT(parent_on_asp);
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occurred so we do not need to call reset().;
          return parent->get_coro();
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

        // Register with parent we have completed this child task.
        if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent therefore, we must continue parent.

          LF_LOG("Task is last child to join, resumes parent");

          if (!parent_on_asp) {
            if (!parent->is_root()) [[likely]] {
              tls::eat(parent);
            }
          }

          // Must reset parents control block before resuming parent.
          parent->reset();

          return parent->get_coro();
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LF_LOG("Task is not last to join");

        if (parent_on_asp) {
          // We are unable to resume the parent, as the resuming thread will take ownership of the parent's stack we must
          // give it up.
          LF_LOG("Thread releases control of parent's stack");
          tls::asp = tls::sbuf.pop()->sentinel();
        }

        LF_ASSERT(tls::asp->is_sentinel());

        return stdx::noop_coroutine();
      }
    };

    return final_awaitable{};
  }

  // public:
  //   template <typename R, typename F, typename... This, typename... Args>
  //   [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<R, tag::fork, F, This...>, Args...> &&packet) {

  //     this->debug_inc();

  //     auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

  //     stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

  //     struct awaitable : stdx::suspend_always {
  //       [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> parent) noexcept ->
  //       decltype(child) {
  //         // In case *this (awaitable) is destructed by stealer after push
  //         stdx::coroutine_handle stack_child = m_child;

  //         LF_LOG("Forking, push parent to context");

  //         Context::context().task_push(task_handle{promise_type::cast_down(parent)});

  //         return stack_child;
  //       }

  //       decltype(child) m_child;
  //     };

  //     return awaitable{{}, child};
  //   }

  //   template <typename R, typename F, typename... This, typename... Args>
  //   [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<R, tag::call, F, This...>, Args...> &&packet) {

  //     this->debug_inc();

  //     auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

  //     stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

  //     struct awaitable : stdx::suspend_always {
  //       [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept
  //           -> decltype(child) {
  //         return m_child;
  //       }
  //       decltype(child) m_child;
  //     };

  //     return awaitable{{}, child};
  //   }

  //   /**
  //    * @brief An invoke should never occur within an async scope as the exceptions will get muddled
  //    */
  // template <typename F, typename... This, typename... Args>
  // [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet) {

  //   FATAL_IN_DEBUG(this->debug_count() == 0, "Invoke within async scope!");

  //   using value_type_child = typename packet<first_arg_t<void, tag::invoke, F, This...>, Args...>::value_type;

  //   using wrapped_value_type =
  //       std::conditional_t<std::is_reference_v<value_type_child>,
  //                          std::reference_wrapper<std::remove_reference_t<value_type_child>>, value_type_child>;

  //   using return_type =
  //       std::conditional_t<std::is_void_v<value_type_child>, regular_void, std::optional<wrapped_value_type>>;

  //   using packet_type = packet<shim_with_context<return_type, Context, first_arg_t<void, tag::invoke, F, This...>>,
  //   Args...>;

  //   using handle_type = typename packet_type::handle_type;

  //   static_assert(std::same_as<value_type_child, typename packet_type::value_type>,
  //                 "An async function's value_type must be return_address_t independent!");

  //   struct awaitable : stdx::suspend_always {

  //     explicit constexpr awaitable(promise_type *in_self,
  //                                  packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet)
  //         : self(in_self),
  //           m_child(packet_type{m_res, {std::move(in_packet.context)}, std::move(in_packet.args)}.invoke_bind(
  //               cast_down(stdx::coroutine_handle<promise_type>::from_promise(*self)))) {}

  //     [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept
  //         -> handle_type {
  //       return m_child;
  //     }

  //     [[nodiscard]] constexpr auto await_resume() -> value_type_child {

  //       LF_ASSERT(self->steals() == 0);

  //       // Propagate exceptions.
  //       if constexpr (LF_PROPAGATE_EXCEPTIONS) {
  //         if constexpr (Tag == tag::root) {
  //           self->get_return_address_obj().exception.rethrow_if_unhandled();
  //         } else {
  //           virtual_stack::from_address(self)->rethrow_if_unhandled();
  //         }
  //       }

  //       if constexpr (!std::is_void_v<value_type_child>) {
  //         LF_ASSERT(m_res.has_value());
  //         return std::move(*m_res);
  //       }
  //     }

  //     return_type m_res;
  //     promise_type *self;
  //     handle_type m_child;
  //   };

  //   return awaitable{this, std::move(in_packet)};
  // }

  // constexpr auto await_transform([[maybe_unused]] join_t join_tag) noexcept {
  //   struct awaitable {
  //   private:
  //     constexpr void take_stack_reset_control() const noexcept {
  //       // Steals have happened so we cannot currently own this tasks stack.
  //       LF_ASSERT(self->steals() != 0);

  //       if constexpr (Tag != tag::root) {

  //         LF_LOG("Thread takes control of task's stack");

  //         Context &context = Context::context();

  //         auto tasks_stack = virtual_stack::from_address(self);
  //         auto thread_stack = context.stack_top();

  //         LF_ASSERT(thread_stack != tasks_stack);
  //         LF_ASSERT(thread_stack->empty());

  //         context.stack_push(tasks_stack);
  //       }

  //       // Some steals have happened, need to reset the control block.
  //       self->reset();
  //     }

  //   public:
  //     [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
  //       // If no steals then we are the only owner of the parent and we are ready to join.
  //       if (self->steals() == 0) {
  //         LF_LOG("Sync ready (no steals)");
  //         // Therefore no need to reset the control block.
  //         return true;
  //       }
  //       // Currently:            joins() = k_imax - num_joined
  //       // Hence:       k_imax - joins() = num_joined

  //       // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
  //       // better if we see all the decrements to joins() and avoid suspending
  //       // the coroutine if possible. Cannot fetch_sub() here and write to frame
  //       // as coroutine must be suspended first.
  //       auto joined = k_imax - self->joins().load(std::memory_order_acquire);

  //       if (self->steals() == joined) {
  //         LF_LOG("Sync is ready");

  //         take_stack_reset_control();

  //         return true;
  //       }

  //       LF_LOG("Sync not ready");
  //       return false;
  //     }

  //     [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> task) noexcept
  //         -> stdx::coroutine_handle<> {
  //       // Currently        joins  = k_imax - num_joined
  //       // We set           joins  = joins() - (k_imax - num_steals)
  //       //                         = num_steals - num_joined

  //       // Hence            joined = k_imax - num_joined
  //       //         k_imax - joined = num_joined

  //       auto steals = self->steals();
  //       auto joined = self->joins().fetch_sub(k_imax - steals, std::memory_order_release);

  //       if (steals == k_imax - joined) {
  //         // We set joins after all children had completed therefore we can resume the task.

  //         // Need to acquire to ensure we see all writes by other threads to the result.
  //         std::atomic_thread_fence(std::memory_order_acquire);

  //         LF_LOG("Wins join race");

  //         take_stack_reset_control();

  //         return task;
  //       }
  //       // Someone else is responsible for running this task and we have run out of work.
  //       LF_LOG("Looses join race");

  //       // We cannot currently own this stack.

  //       if constexpr (Tag != tag::root) {
  //         LF_ASSERT(virtual_stack::from_address(self) != Context::context().stack_top());
  //       }
  //       LF_ASSERT(Context::context().stack_top()->empty());

  //       return stdx::noop_coroutine();
  //     }

  //     constexpr void await_resume() const {
  //       LF_LOG("join resumes");
  //       // Check we have been reset.
  //       LF_ASSERT(self->steals() == 0);
  //       LF_ASSERT(self->joins() == k_imax);

  //       self->debug_reset();

  //       if constexpr (Tag != tag::root) {
  //         LF_ASSERT(virtual_stack::from_address(self) == Context::context().stack_top());
  //       }

  //       // Propagate exceptions.
  //       if constexpr (LF_PROPAGATE_EXCEPTIONS) {
  //         if constexpr (Tag == tag::root) {
  //           self->get_return_address_obj().exception.rethrow_if_unhandled();
  //         } else {
  //           virtual_stack::from_address(self)->rethrow_if_unhandled();
  //         }
  //       }
  //     }

  //     promise_type *self;
  //   };

  //   return awaitable{this};
  // }
};

#undef FATAL_IN_DEBUG

} // namespace lf::detail

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::is_task Task, lf::first_arg Head, typename... Args>
struct lf::stdx::coroutine_traits<Task, Head, Args...> {
  using promise_type =
      lf::detail::promise_type<lf::return_of<Head>, lf::value_of<Task>, lf::context_of<Head>, lf::tag_of<Head>>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::is_task Task, lf::not_first_arg This, lf::first_arg Head, typename... Args>
struct lf::stdx::coroutine_traits<Task, This, Head, Args...> : lf::stdx::coroutine_traits<Task, Head, Args...> {};

#endif /* LF_DOXYGEN_SHOULD_SKIP_THIS */

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
