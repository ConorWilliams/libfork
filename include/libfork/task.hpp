#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>

#include "libfork/detail/allocator.hpp"
#include "libfork/detail/result.hpp"
#include "libfork/detail/wait.hpp"

#include "libfork/unique_handle.hpp"
#include "libfork/utility.hpp"

/**
 * @file task.hpp
 *
 * @brief The task class and associated utilities.
 */

namespace lf {

namespace detail {

template <typename Context, bool Waitable>
class task_handle;

}

/**
 * @brief A triviall handle to a generic task.
 *
 * A work_handle represents ownership/responsibility for running/resuming a generic task.
 *
 * @tparam Context The handle type of the execution context that this task is running on.
 */
template <typename Context>
using work_handle = detail::task_handle<Context, true>;

/**
 * @brief A triviall handle to a root task.
 *
 * A root_handle represents ownership/responsibility for running/resuming a root task.
 *
 * @tparam Context The handle type of the execution context that this task is running on.
 */
using root_handle = detail::task_handle<Context, false>;

/**
 * @brief Defines the interface for an execution context.
 *
 * \rst
 *
 * Specifically:
 *
 * .. code::
 *
 *   concept context = requires(Context context, work_handle<Context> task) {
 *       { context.push(task) } -> std::same_as<void>;
 *       { context.pop() } -> std::convertible_to<std::optional<work_handle<Context>>>;
 *   }
 *
 * \endrst
 */
template <typename Context>
concept context = requires(Context context, work_handle<Context> task) {
                    { context.push(task) } -> std::same_as<void>;
                    { context.pop() } -> std::convertible_to<std::optional<work_handle<Context>>>;
                  };

template <typename T, context Context, typename Allocator = std::allocator<T>, bool Waitable = false>
requires std::negation_v<std::is_void<T>>
class [[nodiscard]] basic_future;

namespace detail {

template <typename T, context Context, typename Allocator, bool Waitable>
struct promise_type;

struct join {};

template <typename P>
struct [[nodiscard]] fork : unique_handle<P> {};

}  // namespace detail

template <typename T, context Context, typename Allocator = std::allocator<T>, bool Waitable = false>
class [[nodiscard]] basic_task;

/**
 * @brief Produce a tag type which when co_awaited will join the current tasks
 * fork-join group.
 */
[[nodiscard]] inline constexpr auto join() noexcept -> detail::join {
  return {};
}

/**
 * @brief Represents a computation that may not have completed.
 */
template <typename T, context Context, typename Allocator, bool Waitable>
requires(Waitable || not std::is_void_v<T>)
class [[nodiscard]] basic_future : private unique_handle<detail::promise_type<T, Context, Allocator, Waitable>> {
 public:
  using value_type = T;                                                        ///< The type of value that this future will return.
  using context_type = Context;                                                ///< The type of execution context that this
  using promise_type = detail::promise_type<T, Context, Allocator, Waitable>;  ///< The type of promise that this future owns.

  constexpr basic_future() noexcept = default;
  /**
   * @brief Construct a new basic_future object from a unique handle.
   */
  constexpr explicit basic_future(unique_handle<promise_type>&& handle) noexcept : unique_handle<promise_type>{std::move(handle)} {}
  /**
   * @brief Wait (block) until the task to complete.
   */
  auto wait() && noexcept -> void
  requires(Waitable && std::is_void_v<T>)
  {
    (*this)->promise().wait();
    // Final suspend responcible for destruction.
    this.release();
  }

  /**
   * @brief Wait (block) until the task to complete.
   */
  auto wait() && noexcept(std::is_nothrow_constructible_v<T, decltype(std::move(local->promise()).get())>) -> T
  requires Waitable
  {
    unique_handle<promise_type> local = std::move(*this);

    local->promise().wait();
    // Safe to access the result, promise destructed by local.
    return std::move(local->promise()).get();
  }

  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() & noexcept -> T&
  requires(!std::is_void_v<T>)
  {
    ASSERT_ASSUME(*this, "future is null");
    return (*this)->promise().get();
  }
  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() && noexcept -> T&&
  requires(!std::is_void_v<T>)
  {
    ASSERT_ASSUME(*this, "future is null");
    return std::move((*this)->promise()).get();
  }
  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() const& noexcept -> T const&
  requires(!std::is_void_v<T>)
  {
    ASSERT_ASSUME(*this, "future is null");
    return (*this)->promise().get();
  }
  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() const&& noexcept -> T const&&
  requires(!std::is_void_v<T>)
  {
    ASSERT_ASSUME(*this, "future is null");
    return std::move((*this)->promise()).get();
  }
};

namespace detail {
/**
 * @brief An alias for ``std::coroutine_handle<T>`
 */
template <typename T = void>
using raw_handle = std::coroutine_handle<T>;

static constexpr int k_imax = std::numeric_limits<int>::max();  ///< Initial value of ``m_join``.

/**
 * @brief The base class which provides, initial-suspend, exception handling and
 * fork-join count.
 */
template <typename Context>
struct promise_base {
  /**
   * @brief Construct a new promise_base base object with a coroutine handle to
   * the derived promise_base.
   */
  constexpr explicit promise_base(raw_handle<> coro) noexcept : m_this{coro} {}
  /**
   * @brief Tasks must be lazy as the parent needs to be pushed onto the
   * contexts's stack.
   */
  [[nodiscard]] constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  /**
   * @brief Called when an exception is thrown in the coroutine and not handled.
   *
   * This will crash the program.
   */
  [[noreturn]] constexpr auto unhandled_exception() const noexcept -> void {
    DEBUG_TRACKER("unhandled_exception");
    std::terminate();
  }

  // Intrinsic
  bool m_is_inline = false;  ///< True if this task is running inline.
  raw_handle<> m_this;       ///< The coroutine handle for this promise.

  // Extrinsic
  promise_base* m_parent{};  ///< To promise of task that spawned this task.
  Context* m_context{};      ///< Pointer to our execution context.

  // State
  int m_steals = 0;                  ///< Number of steals.
  std::atomic<int> m_join = k_imax;  ///< Number of children joined (obfuscated).

#ifndef NDEBUG
  ~promise_base() { DEBUG_TRACKER("~promise_base()"); }
#endif
};

/**
 * @brief A triviall handle to a task.
 *
 * A task_handle represents ownership/responsibility for running/resuming a task.
 *
 * @tparam Context The handle type of the execution context that this task is running on.
 */
template <typename Context, bool Waitable>
class task_handle {
 public:
  /**
   * @brief To make task_handle trivial.
   */
  constexpr task_handle() noexcept = default;

  /**
   * @brief Resume the coroutine associated with this handle.
   *
   * This should be called by the thread owning the execution context that this will is run on.
   */
  constexpr void resume(Context& context) const noexcept {
    ASSERT_ASSUME(m_promise, "resuming null handle");
    ASSERT_ASSUME(m_promise->m_this, "resuming null coroutine");

    if constexpr (Waitable) {
      ASSERT_ASSUME(!m_promise->m_parent, "not a root task");
      ASSERT_ASSUME(!m_promise->m_context, "root tasks should not have a context");

    } else {
      ASSERT_ASSUME(m_promise->m_context, "resuming stolen handle with null context");
      ASSERT_ASSUME(m_promise->m_context != std::addressof(context), "bad steal call");

      m_promise->m_steals += 1;
    }

    DEBUG_TRACKER("resume sets context");

    m_promise->m_context = std::addressof(context);
    m_promise->m_this.resume();
  }

 private:
  template <typename, context, typename>
  friend struct detail::promise_type;

  template <typename, context, typename>
  friend class basic_task;

  detail::promise_base<Context>* m_promise;  ///< The promise associated with this handle.

  /**
   * @brief Construct a handle to a promise.
   */
  constexpr explicit task_handle(detail::promise_base<Context>& coro) noexcept : m_promise{std::addressof(coro)} {}
};

// A minimal context for static-assert.
struct minimal_context {
  auto push(work_handle<minimal_context>) -> void;            // NOLINT
  auto pop() -> std::optional<work_handle<minimal_context>>;  // NOLINT
};

static_assert(std::is_trivial_v<task_handle<minimal_context, true>>);
static_assert(std::is_trivial_v<task_handle<minimal_context, false>>);

/**
 * @brief The promise type for a basic_task.
 */
template <typename T, context Context, typename Allocator, bool Waitable>
struct promise_type : detail::allocator_mixin<Allocator>, result<T>, waiter<Waitable> promise_base<Context> {
  /**
   * @brief Construct a new promise type object.
   */
  constexpr promise_type() noexcept : promise_base<Context>{raw_handle<promise_type>::from_promise(*this)} {}
  /**
   * @brief This is the object returned when a basic_task is created by a
   * function call.
   */
  [[nodiscard]] constexpr auto get_return_object() noexcept -> basic_task<T, Context, Allocator, Waitable> {
    //
    return basic_task<T, Context, Allocator, Waitable>{raw_handle<promise_type>::from_promise(*this)};
  }

  /**
   * @brief Called at end of coroutine frame.
   *
   * Resumes parent task if we are the last child.
   */
  [[nodiscard]] constexpr auto final_suspend() const noexcept {
    struct final_awaitable : std::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type> child) const noexcept -> raw_handle<> {
        //
        promise_type const& prom = child.promise();

        DEBUG_TRACKER("task reaches final suspend");

        ASSERT_ASSUME(prom.m_context, "execution context not set");
        ASSERT_ASSUME(prom.m_steals == 0, "fork without join");
        ASSERT_ASSUME(prom.m_join.load() == k_imax, "promise destroyed in invalid state");

        if constexpr (Waitable) {
          DEBUG_TRACKER("waitable task at final suspend");

          ASSER_ASSUME(!prom.m_parent, "waitable task is not a root task");

          if constexpr (std::is_void_v<T>) {
            // As soon as we call release() we must assume that the promise is deallocated.
          }

          // Future is responsible for destroying the promise.
          return std::noop_coroutine();
        }

        if (prom.m_is_inline) {
          DEBUG_TRACKER("inline task resumes parent and sets context");

          ASSERT_ASSUME(prom.m_parent, "inline task has no parent");
          ASSERT_ASSUME(prom.m_parent->m_this, "inline task's parents this pointer not set");

          prom.m_parent->m_context = prom.m_context;  // in-case we stole an inline task

          return destroy_if_void(child, prom.m_parent->m_this);
        }

        if (std::optional parent_handle = prom.m_context->pop()) {
          // No-one stole continuation, we are the exclusive owner of parent,
          // just keep rippin!
          DEBUG_TRACKER("fast path, keeps ripping");

          ASSERT_ASSUME(prom.m_parent, "parent is null -> task is root but, pop() non-null");
          ASSERT_ASSUME(parent_handle->m_promise->m_this, "parent's this handle is null");
          ASSERT_ASSUME(parent_handle->m_promise->m_context == prom.m_context, "context changed");
          ASSERT_ASSUME(prom.m_parent->m_this == parent_handle->m_promise->m_this, "pop should have got the same");

          return destroy_if_void(child, parent_handle->m_promise->m_this);
        }

        DEBUG_TRACKER("task's parent was stolen");

        ASSERT_ASSUME(prom.m_parent, "parent is null");

        // Register with parent we have completed this child task.
        if (prom.m_parent->m_join.fetch_sub(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent.
          // Hence, we should continue parent, therefore we must set the parents
          // context.

          DEBUG_TRACKER("task is last child to join, sets context and resumes parent");

          ASSERT_ASSUME(prom.m_parent->m_this, "parent's this handle is null");

          prom.m_parent->m_context = prom.m_context;

          return destroy_if_void(child, prom.m_parent->m_this);
        }
        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, yield to executor.
        DEBUG_TRACKER("task is not last to join");
        return destroy_if_void(child, std::noop_coroutine());
      }
    };
    return final_awaitable{};
  }

  template <typename U, typename Alloc>
  [[nodiscard]] constexpr auto await_transform(fork<promise_type<U, Context, Alloc, false>>&& child) noexcept {
    //
    struct awaitable : unique_handle<promise_type<U, Context, Alloc, false>> {
      //
      [[nodiscard]] static constexpr auto await_ready() noexcept -> bool { return false; }

      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type> parent) noexcept -> raw_handle<> {
        // In case *this (awaitable) is destructed by stealer after push
        raw_handle<> child = (*this)->promise().m_this;

        DEBUG_TRACKER("forking, push parent to context");

        ASSERT_ASSUME((*this)->promise().m_context == parent.promise().m_context, "child is not in same context as parent");

        parent.promise().m_context->push(work_handle<Context>{parent.promise()});

        return child;
      }

      constexpr auto await_resume() noexcept {
        if constexpr (std::is_void_v<U>) {
          // Promise cleaned up at final_suspend.
          DEBUG_TRACKER("releasing void promise");
          this->release();
          return;
        } else {
          return basic_future<U, Context, Alloc, false>{std::move(*this)};
        }
      }
    };

    DEBUG_TRACKER("forking child context");

    ASSERT_ASSUME(child, "fork child is null");
    ASSERT_ASSUME(!child->promise().m_is_inline, "forking inline task");

    child->promise().m_parent = this;
    child->promise().m_context = this->m_context;

    return awaitable{std::move(child)};
  }

  template <typename U, typename Alloc>
  [[nodiscard]] constexpr auto await_transform(basic_task<U, Context, Alloc, false>&& child) noexcept {
    //
    struct awaitable : unique_handle<promise_type<U, Context, Alloc, false>> {
      //
      [[nodiscard]] static constexpr auto await_ready() noexcept -> bool { return false; }

      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type> parent) noexcept -> raw_handle<> {  // NOLINT
        DEBUG_TRACKER("launching inline task");

        ASSERT_ASSUME((*this)->promise().m_context == parent.promise().m_context, "inline child is not in same context as parent");

        return (*this)->promise().m_this;
      }

      constexpr auto await_resume() noexcept -> void
      requires(std::is_void_v<U>)
      {
        // Promise cleaned up at final_suspend.
        DEBUG_TRACKER("releasing void promise");
        this->release();
        return;
      }

      constexpr auto await_resume() noexcept(std::is_nothrow_constructible_v<U, decltype(std::move((*this)->promise()).get())>) -> U
      requires(!std::is_void_v<U>)
      {
        return std::move((*this)->promise()).get();
        //
      }
    };

    DEBUG_TRACKER("inline child sets context");

    ASSERT_ASSUME(child, "inline child is null");

    child->promise().m_is_inline = true;
    child->promise().m_parent = this;
    child->promise().m_context = this->m_context;

    return awaitable{std::move(child)};
  }

  auto await_transform(join) noexcept {  // NOLINT
    struct awaitable {
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // Currently:            m_join = k_imax - num_joined
        // Hence:       k_imax - m_join = num_joined

        // If no steals then we are the only owner of the parent and we are
        // ready to join.
        if (m_promise->m_steals == 0) {
          DEBUG_TRACKER("sync() ready (no steals)");
          return true;
        }
        // Could use (relaxed) + (fence(acquire) in truthy branch) but, its
        // better if we see all the decrements to m_join and avoid suspending
        // the coroutine if possible.
        auto joined = k_imax - m_promise->m_join.load(std::memory_order_acquire);

        if (m_promise->m_steals == joined) {
          DEBUG_TRACKER("sync() is ready");
          return true;
        }
        DEBUG_TRACKER("sync() not ready");
        return false;
      }

      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type> task) noexcept -> raw_handle<> {
        // Currently        m_join = k_imax - num_joined
        // We set           m_join = m_join - (k_imax - num_steals)
        //                         = num_steals - num_joined

        // Hence            joined = k_imax - num_joined
        //         k_imax - joined = num_joined

        //  Consider race condition on write to m_context.

        ASSERT(&task.promise() == m_promise, "logic error, task has changed");

        auto steals = m_promise->m_steals;
        auto joined = m_promise->m_join.fetch_sub(k_imax - steals, std::memory_order_release);

        if (steals == k_imax - joined) {
          // We set n after all children had completed therefore we can resume
          // task.

          // Need to acquire to ensure we see all writes by other threads to the
          // result.
          std::atomic_thread_fence(std::memory_order_acquire);

          DEBUG_TRACKER("sync() wins");
          return task;
        }
        // Someone else is responsible for running this task and we have run out
        // of work.
        DEBUG_TRACKER("sync() looses");
        return std::noop_coroutine();
      }

      constexpr void await_resume() const noexcept {
        // After a sync we reset a/n
        m_promise->m_steals = 0;
        // We know we are the only thread who can touch this promise until a
        // steal which would provide the required memory syncronisation.
        m_promise->m_join.store(k_imax, std::memory_order_relaxed);
      }

      promise_base<Context>* m_promise;
    };

    return awaitable{this};
  }

 private:
  /**
   * @brief Destroy a non-null handle to a void basic_task.
   */
  template <typename P>
  static inline auto destroy_if_void(raw_handle<promise_type> to_destroy, raw_handle<P> fwd) noexcept {
    //
    ASSERT_ASSUME(to_destroy, "attempting to destroy null handle");

    if constexpr (std::is_void_v<T>) {
      DEBUG_TRACKER("call to destroy void");
      to_destroy.destroy();
    } else {
      DEBUG_TRACKER("call to destroy_if_void() elided");
    }

    return fwd;
  }
};

}  // namespace detail

/**
 * @brief ...
 *
 * @tparam T
 * @tparam Context
 */
template <typename T, context Context, typename Allocator, bool Waitable>
class [[nodiscard]] basic_task : private unique_handle<detail::promise_type<T, Context, Allocator, Waitable>> {
 public:
  using value_type = T;                                                        ///< The type of value that this task will return.
  using handle_type = task_handle<Context>;                                    ///< The type of handle that this task will use.
  using context_type = Context;                                                ///< The type of execution context that this task will run on.
  using promise_type = detail::promise_type<T, Context, Allocator, Waitable>;  ///< The type of promise that
                                                                               ///< this task will use.

  /**
   * @brief A named tuple returned by ``get_handle()``.
   *
   * In the case of a void task the ``future`` member will be ``regular_void``.
   * This enables consistent structured bindings.
   */
  struct named_pair {
    basic_future<T, Context, Allocator, true> future;  ///< Future to result.
    handle_type handle;                                ///< Resumable handle to task.
  };

  /**
   * @brief Decompose this task into a future and a handle and promise to call
   * resume on the handle.
   *
   * This requires the task to embed a notification mechanism to allow the
   * caller to know when the future is ready.
   */
  [[nodiscard]] constexpr auto make_promise() && noexcept -> named_pair
  requires Waitable
  {
    DEBUG_TRACKER("make_promise()");

    ASSERT_ASSUME(*this, "attempting to get handle from null task");

    handle_type const hand{(*this)->promise()};

    if constexpr (std::is_void_v<T>) {
      DEBUG_TRACKER("releasing void from make_promise()");
      this->release();
    }

    return {
        .future = basic_future<T, Context, Allocator, Waitable>{std::move(*this)},
        .handle = hand,
    };
  }

  /**
   * @brief Get an awaitbale which will cause the current task to fork.
   */
  [[nodiscard]] constexpr auto fork() && noexcept -> detail::fork<promise_type> {
    ASSERT_ASSUME(*this, "forking a null task");
    return {std::move(*this)};
  }

#ifndef NDEBUG  // Rule of zero in release.
  constexpr ~basic_task() noexcept { ASSERT_ASSUME(!*this, "task destructed without co_await"); }
#endif

 private:
  template <typename, context, typename>
  friend struct detail::promise_type;

  constexpr explicit basic_task(detail::raw_handle<promise_type> handle) : unique_handle<promise_type>{handle} {}
};

}  // namespace lf