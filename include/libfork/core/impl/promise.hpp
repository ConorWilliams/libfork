#ifndef C854CDE9_1125_46E1_9E2A_0B0006BFC135
#define C854CDE9_1125_46E1_9E2A_0B0006BFC135

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <new>
#include <type_traits>
#include <utility>

#include <libfork/core/co_alloc.hpp>
#include <libfork/core/control_flow.hpp>
#include <libfork/core/first_arg.hpp>
#include <libfork/core/invocable.hpp>
#include <libfork/core/tag.hpp>
#include <libfork/core/task.hpp>

#include <libfork/core/ext/context.hpp>
#include <libfork/core/ext/handles.hpp>
#include <libfork/core/ext/tls.hpp>

#include <libfork/core/impl/awaitables.hpp>
#include <libfork/core/impl/combinate.hpp>
#include <libfork/core/impl/frame.hpp>
#include <libfork/core/impl/return.hpp>
#include <libfork/core/impl/utility.hpp>

/**
 * @file promise.hpp
 *
 * @brief The promise type for all tasks/coroutines.
 */

namespace lf::impl {

namespace detail {

inline auto final_await_suspend(frame *parent) noexcept -> std::coroutine_handle<> {

  full_context *context = tls::context();

  if (task_handle parent_task = context->pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
    LF_LOG("Parent not stolen, keeps ripping");
    LF_ASSERT(byte_cast(parent_task) == byte_cast(parent));
    // This must be the same thread that created the parent so it already owns the fibre.
    // No steals have occurred so we do not need to call reset().;
    return parent->self();
  }

  /**
   * An owner is a worker who:
   *
   * - Created the task.
   * - Had the task submitted to them.
   * - Won the task at a join.
   *
   * An owner of a task owns the fibre the task is on.
   *
   * As the worker who completed the child task this thread owns the fibre the child task was on.
   *
   * Either:
   *
   * 1. The parent is on the same fibre as the child.
   * 2. The parent is on a different fibre to the child.
   *
   * Case (1) implies: we owned the parent; forked the child task; then the parent was then stolen.
   * Case (2) implies: we stole the parent task; then forked the child; then the parent was stolen.
   *
   * In case (2) the workers fibre has no allocations on it.
   */

  LF_LOG("Task's parent was stolen");

  fibre *tls_fibre = tls::fibre();

  auto *parents_fibril = parent->fibril();

  // Register with parent we have completed this child task.
  if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
    // Acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    // Parent has reached join and we are the last child task to complete.
    // We are the exclusive owner of the parent therefore, we must continue parent.

    LF_LOG("Task is last child to join, resumes parent");

    if (parents_fibril != tls_fibre->top()) {
      // Case (2), the tls_fibre has no allocations on it.

      // TODO: fibre.splice()? Here the old fibre is empty and thrown away, if it is larger
      // then we could splice it onto the parents one? Or we could attempt to cache the old one.
      *tls_fibre = fibre{parents_fibril};
    }

    // Must reset parents control block before resuming parent.
    parent->reset();

    return parent->self();
  }

  // We did not win the join-race, we cannot deference the parent pointer now as
  // the frame may now be freed by the winner.

  // Parent has not reached join or we are not the last child to complete.
  // We are now out of jobs, must yield to executor.

  LF_LOG("Task is not last to join");

  if (parents_fibril == tls_fibre->top()) {
    // We are unable to resume the parent and where its owner, as the resuming
    // thread will take ownership of the parent's we must give it up.
    LF_LOG("Thread releases control of parent's stack");

    ignore_t{} = tls_fibre->release();

  } else {
    // Case (2) the tls_fibre has no allocations on it, it may be used later.
  }

  return std::noop_coroutine();
}

} // namespace detail

/**
 * @brief Type independent bits
 */
struct promise_base : frame {

  using frame::frame;

  /**
   * @brief Allocate the coroutine on a new fibre.
   */
  LF_FORCEINLINE static auto operator new(std::size_t size) -> void * { return tls::fibre()->allocate(size); }

  /**
   * @brief Deallocate the coroutine from current `fibre`s stack.
   */
  LF_FORCEINLINE static void operator delete(void *ptr) noexcept { tls::fibre()->deallocate(ptr); }

  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  /**
   * @brief Terminates the program.
   */
  static void unhandled_exception() noexcept {
    noexcept_invoke([] {
      LF_RETHROW;
    });
  }

  // -------------------------------------------------------------- //

  template <co_allocable T, std::size_t E>
  auto await_transform(co_new_t<T, E> await) {

    auto *fibre = tls::fibre();

    T *ptr = static_cast<T *>(fibre->allocate(await.count * sizeof(T)));

    // clang-format off

    LF_TRY {
      std::ranges::uninitialized_default_construct_n(ptr, await.count);
    } LF_CATCH_ALL {
      fibre->deallocate(ptr);
      LF_RETHROW;
    }

    // clang-format on

    this->set_fibril(fibre->top());

    return alloc_awaitable<T, E>{{}, std::span<T, E>{ptr, await.count}};
  }

  template <co_allocable T, std::size_t E>
  auto await_transform(co_delete_t<T, E> await) noexcept -> std::suspend_never {
    std::ranges::destroy(await);
    auto *fibre = impl::tls::fibre();
    fibre->deallocate(await.data());
    this->set_fibril(fibre->top());
    return {};
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Transform a context pointer into a context-switch awaitable.
   */
  auto await_transform(context *dest) -> switch_awaitable {

    auto *submit = std::bit_cast<submit_handle>(static_cast<frame *>(this));

    return {{}, typename intrusive_list<submit_handle>::node{submit}, dest};
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Get a join awaitable.
   */
  auto await_transform(join_type) noexcept -> join_awaitable { return {this}; }

  // -------------------------------------------------------------- //

  /**
   * @brief Transform a call packet into a call awaitable.
   */
  template <returnable R2, return_address_for<R2> I2, tag Tg>
    requires (Tg == tag::call || Tg == tag::fork)
  auto await_transform(quasi_awaitable<R2, I2, Tg> awaitable) noexcept {

    awaitable.promise->set_parent(this);

    if constexpr (Tg == tag::call) {
      return call_awaitable{{}, awaitable.promise};
    }

    if constexpr (Tg == tag::fork) {
      return fork_awaitable{{}, awaitable.promise, this};
    }
  }
};

/**
 * @brief The promise type for all tasks/coroutines.
 *
 * @tparam R The type of the return address.
 * @tparam T The value type of the coroutine (what it promises to return).
 * @tparam Context The type of the context this coroutine is running on.
 * @tparam Tag The dispatch tag of the coroutine.
 */
template <returnable R, return_address_for<R> I, tag Tag>
struct promise : promise_base, return_result<R, I> {

  promise() noexcept
      : promise_base{std::coroutine_handle<promise>::from_promise(*this), tls::fibre()->top()} {}

  auto get_return_object() noexcept -> task<R> { return {{}, static_cast<void *>(this)}; }

  /**
   * @brief Try to resume the parent.
   */
  auto final_suspend() const noexcept {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the fibre_stack this child is on

    LF_ASSERT(this->load_steals() == 0);                                           // Fork without join.
    LF_ASSERT_NO_ASSUME(this->load_joins(std::memory_order_acquire) == k_u16_max); // Invalid state.

    return final_awaitable{};
  }

 private:
  struct final_awaitable : std::suspend_always {
    static auto await_suspend(std::coroutine_handle<promise> child) noexcept -> std::coroutine_handle<> {

      if constexpr (Tag == tag::root) {

        LF_LOG("Root task at final suspend, releases semaphore and yields");

        child.promise().semaphore()->release();
        child.destroy();

        // A root task is always the first on a fibre, now it has been completed the fibre is empty.

        return std::noop_coroutine();
      }

      LF_LOG("Task reaches final suspend, destroying child");

      frame *parent = child.promise().parent();
      child.destroy();

      if constexpr (Tag == tag::call) {
        LF_LOG("Inline task resumes parent");
        // Inline task's parent cannot have been stolen as its continuation was not
        // pushed to a queue hence, no need to reset control block. We do not
        // attempt to take the fibre because stack-eats only occur at a sync point.
        return parent->self();
      }

      return detail::final_await_suspend(parent);
    }
  };
};

// -------------------------------------------------- //

template <typename...>
inline constexpr bool always_false = false;

/**
 * @brief All non-reference destinations are safe for most types.
 */
template <tag Tag, typename, typename To>
struct safe_fork_t : std::true_type {};

/**
 * @brief Rvalue to const-lvalue promotions are unsafe.
 */
template <typename From, typename To>
struct safe_fork_t<tag::fork, From &&, To const &> : std::false_type {
  static_assert(always_false<From &&, To const &>, "Unsafe r-value to const l-value conversion may dangle!");
};

/**
 * @brief All r-value destinations are always unsafe.
 */
template <typename From, typename To>
struct safe_fork_t<tag::fork, From, To &&> : std::false_type {
  static_assert(always_false<From, To &&>, "Forked r-value may dangle!");
};

template <tag Tag, typename From, typename To>
inline constexpr bool safe_fork_v = safe_fork_t<Tag, From, To>::value;

} // namespace lf::impl

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::returnable R,
          lf::impl::return_address_for<R> I,
          lf::tag Tag,
          lf::async_function_object F,
          typename... Crgs,
          typename... Args>
struct std::coroutine_traits<lf::task<R>, lf::impl::first_arg_t<I, Tag, F, Crgs...>, Args...> {
  // This will trigger an inner static assert if an unsafe reference is forked.
  static_assert((lf::impl::safe_fork_v<Tag, Crgs, Args> && ...));

  using promise_type = lf::impl::promise<R, I, Tag>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::returnable R,
          typename This,
          lf::impl::return_address_for<R> I,
          lf::tag Tag,
          lf::async_function_object F,
          typename... Crgs,
          typename... Args>
struct std::coroutine_traits<lf::task<R>, This, lf::impl::first_arg_t<I, Tag, F, Crgs...>, Args...> {
  // This will trigger an inner static assert if an unsafe reference is forked.
  static_assert((lf::impl::safe_fork_v<Tag, Crgs, Args> && ...));
  static_assert((lf::impl::safe_fork_v<Tag, This, This>), "Object parameter will dangle!");

  using promise_type = lf::impl::promise<R, I, Tag>;
};

// TODO: test if disallowing r-values for forked coroutines at the top level breaks concepts.

#endif /* C854CDE9_1125_46E1_9E2A_0B0006BFC135 */
