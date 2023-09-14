#ifndef FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0
#define FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <coroutine>
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

#include "libfork/core/call.hpp"
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
      : allocator<Tag>{std::coroutine_handle<>{handle_t::from_promise(*this)}},
        promise_result<R, T>{head.address()} {}

  template <not_first_arg Self, first_arg Head, typename... Tail>
  constexpr promise_type([[maybe_unused]] Self const &self, Head const &head, Tail &&...tail) noexcept
    requires std::constructible_from<promise_result<R, T>, R *>
      : promise_type{head, std::forward<Tail>(tail)...} {}

  constexpr promise_type() noexcept : allocator<Tag>(handle_t::from_promise(*this)) {}

  auto get_return_object() noexcept -> frame_block * { return this; }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  void unhandled_exception() noexcept {
    noexcept_invoke([] { LF_RETHROW; });
  }

  auto final_suspend() noexcept {

    // Completing a non-root task means we currently own the async_stack this child is on

    FATAL_IN_DEBUG(this->debug_count() == 0, "Fork/Call without a join!");

    LF_ASSERT(this->steals() == 0);                                      // Fork without join.
    LF_ASSERT(this->load_joins(std::memory_order_acquire) == k_u32_max); // Destroyed in invalid state.

    struct final_awaitable : stdx::suspend_always {
      constexpr auto await_suspend(stdx::coroutine_handle<promise_type> child) const noexcept -> stdx::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LF_LOG("Root task at final suspend, releases semaphore");
          // Finishing a root task implies our stack is empty and should have no exceptions.
          child.promise().address()->semaphore.release();
          child.destroy();
          LF_LOG("Root task yields to executor");
          return stdx::noop_coroutine();
        }

        LF_LOG("Task reaches final suspend");

        frame_block *parent = child.promise().parent();
        child.destroy();

        LF_ASSERT(parent);

        if constexpr (Tag == tag::call) {
          LF_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent->coro();
        }

        Context *context = tls::ctx<Context>;

        LF_ASSERT(context);

        if (frame_block *parent_task = context->task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LF_LOG("Parent not stolen, keeps ripping");
          LF_ASSERT(parent_task == parent);
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occurred so we do not need to call reset().;
          return parent->coro();
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

          if (!parent->is_root()) [[likely]] {
            if (parent->top() != tls::asp) {
              tls::eat<Context>(parent->top());
            }
          }

          // Must reset parents control block before resuming parent.
          parent->reset();

          return parent->coro();
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LF_LOG("Task is not last to join");

        if (parent->top() == tls::asp) {
          // We are unable to resume the parent, as the resuming thread will take
          // ownership of the parent's stack we must give it up.
          LF_LOG("Thread releases control of parent's stack");
          tls::asp = context->stack_pop()->as_bytes();
        }

        return stdx::noop_coroutine();
      }
    };

    return final_awaitable{};
  }

  template <first_arg Head, typename... Args>
    requires(tag_of<Head> == tag::fork)
  [[nodiscard]] constexpr auto await_transform(packet<Head, Args...> &&packet)
    requires requires { std::move(packet).template patch_with<Context>(); }
  {

    this->debug_inc();

    frame_block *child = std::move(packet).template patch_with<Context>().invoke(this);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        LF_LOG("Forking, push parent to context");

        auto *child = m_child; // Need it here (on real stack) in case *this is destructed after push.

        tls::ctx<Context>->task_push(m_parent);

        return child->coro();
      }

      frame_block *m_parent;
      frame_block *m_child;
    };

    return awaitable{{}, this, child};
  }

  template <first_arg Head, typename... Args>
    requires(tag_of<Head> == tag::call)
  [[nodiscard]] constexpr auto await_transform(packet<Head, Args...> &&packet)
    requires requires { std::move(packet).template patch_with<Context>(); }
  {

    frame_block *child = std::move(packet).template patch_with<Context>().invoke(this);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        LF_LOG("Calling");
        return m_child->coro();
      }

      frame_block *m_child;
    };

    return awaitable{{}, child};
  }

  template <typename F, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<basic_first_arg<void, tag::invoke, F>, Args...> &&packet)
    requires std::is_void_v<value_of<lf::packet<basic_first_arg<void, tag::invoke, F>, Args...>>>
  {

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        using new_packet_t = lf::packet<basic_first_arg<void, tag::call, F>, Args...>;

        new_packet_t new_packet = std::move(m_packet).apply([&](auto, Args &&...args) -> new_packet_t {
          return {{}, std::forward<Args>(args)...};
        });

        static_assert(std::is_void_v<value_of<new_packet_t>>, "Value type dependant on first arg!");

        LF_LOG("Invoking");

        return std::move(new_packet).template patch_with<Context>().invoke(parent)->coro();
      }

      frame_block *parent;
      lf::packet<basic_first_arg<void, tag::invoke, F>, Args...> m_packet;
    };

    return awaitable{{}, this, std::move(packet)};
  }

  template <typename F, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<basic_first_arg<void, tag::invoke, F>, Args...> &&packet) {

    using packet_t = lf::packet<basic_first_arg<void, tag::invoke, F>, Args...>;
    using return_t = eventually<value_of<packet_t>>;

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        using new_packet_t = lf::packet<basic_first_arg<return_t, tag::call, F>, Args...>;

        new_packet_t new_packet = std::move(m_packet).apply([&](auto, Args &&...args) -> new_packet_t {
          return {{m_res}, std::forward<Args>(args)...};
        });

        static_assert(std::same_as<value_of<packet_t>, value_of<new_packet_t>>, "Value type dependant on first arg!");

        LF_LOG("Invoking");

        return std::move(new_packet).template patch_with<Context>().invoke(parent)->coro();
      }

      [[nodiscard]] constexpr auto await_resume() -> value_of<packet_t> { return *std::move(m_res); }

      frame_block *parent;
      packet_t m_packet;
      return_t m_res;
    };

    return awaitable{{}, this, std::move(packet), {}};
  }

  constexpr auto await_transform([[maybe_unused]] join_type join_tag) noexcept {
    struct awaitable {
    private:
      constexpr void take_stack_reset_control() const noexcept {
        // Steals have happened so we cannot currently own this tasks stack.
        LF_ASSERT(self->steals() != 0);

        if constexpr (Tag != tag::root) {
          tls::eat<Context>(self->top());
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
        // Currently:            joins() = k_u32_max - num_joined
        // Hence:       k_u32_max - joins() = num_joined

        // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
        // better if we see all the decrements to joins() and avoid suspending
        // the coroutine if possible. Cannot fetch_sub() here and write to frame
        // as coroutine must be suspended first.
        auto joined = k_u32_max - self->load_joins(std::memory_order_acquire);

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
        // Currently        joins  = k_u32_max  - num_joined
        // We set           joins  = joins()    - (k_u32_max - num_steals)
        //                         = num_steals - num_joined

        // Hence               joined = k_u32_max - num_joined
        //         k_u32_max - joined = num_joined

        auto steals = self->steals();
        auto joined = self->fetch_sub_joins(k_u32_max - steals, std::memory_order_release);

        if (steals == k_u32_max - joined) {
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
          LF_ASSERT(self->top() != tls::asp);
        }

        return stdx::noop_coroutine();
      }

      constexpr void await_resume() const noexcept {
        LF_LOG("join resumes");
        // Check we have been reset.
        LF_ASSERT(self->steals() == 0);
        LF_ASSERT(self->load_joins(std::memory_order_relaxed) == k_u32_max);

        self->debug_reset();

        if constexpr (Tag != tag::root) {
          LF_ASSERT(self->top() == tls::asp);
        }
      }

      frame_block *self;
    };

    return awaitable{this};
  }
};

#undef FATAL_IN_DEBUG

} // namespace lf::detail

namespace lf {
/**
 * @brief Disable rvalue references for T&& template types if an async function is forked.
 *
 * This is to prevent the user from accidentally passing a temporary object to
 * an async function that will then destructed in the parent task before the
 * child task returns.
 */
template <typename T, tag Tag>

concept no_dangling = Tag != tag::fork || !std::is_rvalue_reference_v<T>;

template <first_arg Head, lf::is_task Task>
using promise_for = detail::promise_type<return_of<Head>, value_of<Task>, context_of<Head>, tag_of<Head>>;

} // namespace lf

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::is_task Task, lf::first_arg Head, lf::no_dangling<lf::tag_of<Head>>... Args>
struct lf::stdx::coroutine_traits<Task, Head, Args...> {
  using promise_type = lf::promise_for<Head, Task>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::is_task Task, lf::not_first_arg This, lf::first_arg Head, lf::no_dangling<lf::tag_of<Head>>... Args>
struct lf::stdx::coroutine_traits<Task, This, Head, Args...> : lf::stdx::coroutine_traits<Task, Head, Args...> {};

#endif /* LF_DOXYGEN_SHOULD_SKIP_THIS */

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
