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
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>

#include "libfork/result.hpp"
#include "libfork/unique_handle.hpp"
#include "libfork/utility.hpp"

/**
 * @file basic_task.hpp
 *
 * @brief The task class and associated utilities.
 */

namespace lf {

/**
 * @brief A regular type which indicates the absence of a return value.
 */
struct regular_void {};

template <typename T>
class future;

template <typename Context>
class task_handle;

/**
 * @brief Defines the interface for an execution context.
 *
 * \rst
 *
 * Specifically:
 *
 * .. include:: ../../include/libfork/task.hpp
 *    :code:
 *    :start-line: 61
 *    :end-before: //! END-CONTEXT-CAPTURE
 *
 * \endrst
 */
template <typename Context>
concept context = requires(Context context, task_handle<Context> task) {
                    { context.push(task) } -> std::same_as<void>;
                    { context.pop() } -> std::convertible_to<std::optional<task_handle<Context>>>;
                  };
//! END-CONTEXT-CAPTURE

namespace detail {

template <typename T, context Context>
struct promise_type;

struct join {};

template <typename P>
struct fork : unique_handle<P> {};

template <typename P>
struct just : unique_handle<P> {};

}  // namespace detail

template <typename T, context Context>
class [[nodiscard]] basic_task;

/**
 * @brief Produce a tag type which when co_awaited will join the current tasks fork-join group.
 */
[[nodiscard]] inline constexpr auto join() noexcept -> detail::join {
  return {};
}

/**
 * @brief Represents a computation that may not have completed.
 */
template <typename T>
class future : unique_handle<T> {
 public:
  using unique_handle<T>::unique_handle;  ///< Inherit the constructors from ``unique_handle``.

  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() & noexcept -> T& { return this->promise().get(); }
  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() && noexcept -> T&& { return std::move(this->promise()).get(); }
  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() const& noexcept -> T const& { return this->promise().get(); }
  /**
   * @brief Access the result of the task.
   */
  [[nodiscard]] constexpr auto operator*() const&& noexcept -> T const&& { return std::move(this->promise()).get(); }
};

namespace detail {

/**
 * @brief An alias for ``std::coroutine_handle<T>`
 */
template <typename T = void>
using raw_handle = std::coroutine_handle<T>;

static constexpr int k_imax = std::numeric_limits<int>::max();  ///< Initial value of ``m_join``.

/**
 * @brief The base class which provides, initial-suspend, exception handling and fork-join count.
 */
template <typename Context>
struct promise_base {
  /**
   * @brief Construct a new promise_base base object with a coroutine handle to the derived
   * promise_base.
   */
  constexpr explicit promise_base(raw_handle<> coro) noexcept : m_this{coro} {}
  /**
   * @brief Tasks must be lazy as the parent needs to be pushed onto the contexts's stack.
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
};

}  // namespace detail

/**
 * @brief A triviall handle to a task.
 *
 * A task_handle represents ownership/responsibility for running/resuming a task.
 *
 * @tparam Context The handle type of the execution context that this task is running on.
 */
template <typename Context>
class task_handle {
 public:
  /**
   * @brief To make task_handle trivial.
   */
  constexpr task_handle() noexcept = default;

  /**
   * @brief Construct a handle to a promise.
   */
  constexpr explicit task_handle(detail::promise_base<Context>& coro) noexcept : m_promise{&coro} {}

  /**
   * @brief Resume the coroutine associated with this handle.
   *
   * This should be called by the thread owning the execution context that this will is run on.
   * Furthermore this task should be a root task.
   */
  constexpr void resume_root(Context& context) const noexcept {
    ASSERT_ASSUME(m_promise, "resuming null handle");
    ASSERT_ASSUME(m_promise->m_this, "resuming null coroutine");
    ASSERT_ASSUME(!m_promise->m_parent, "not a root task");
    ASSERT_ASSUME(!m_promise->m_context, "root tasks should not have a context");
    m_promise->m_context = std::addressof(context);
    m_promise->m_this.resume();
  }

  /**
   * @brief Resume the coroutine associated with this handle.
   *
   * This should be called on a handle stolen from another context.
   */
  constexpr void resume_stolen(Context& context) const noexcept {
    ASSERT_ASSUME(m_promise, "resuming null handle");
    ASSERT_ASSUME(m_promise->m_context, "resuming stolen handle with null context");
    ASSERT_ASSUME(m_promise->m_context != std::addressof(context), "bad steal call");
    ASSERT_ASSUME(m_promise->m_this, "resuming null coroutine");
    m_promise->m_context = std::addressof(context);
    m_promise->m_steals += 1;
    m_promise->m_this.resume();
  }

 private:
  template <typename, context>
  friend class detail::promise_type;

  detail::promise_base<Context>* m_promise;  ///< The promise associated with this handle.
};

namespace detail {

// A minimal context for static-assert.
struct minimal_context {
  void push(task_handle<minimal_context>);            // NOLINT
  std::optional<task_handle<minimal_context>> pop();  // NOLINT
};

static_assert(std::is_trivial_v<task_handle<minimal_context>>);

/**
 * @brief The promise type for a basic_task.
 */
template <typename T, context Context>
struct promise_type : promise_base<Context>, result<T> {
  /**
   * @brief Construct a new promise type object.
   */
  constexpr promise_type() noexcept : promise_base<Context>{raw_handle<promise_type>::from_promise(*this)} {}
  /**
   * @brief This is the object returned when a basic_task is created by a function call.
   */
  [[nodiscard]] constexpr auto get_return_object() noexcept -> basic_task<T, Context> {
    //
    return basic_task<T, Context>{raw_handle<promise_type>::from_promise(*this)};
  }

  /**
   * @brief Called at end of coroutine frame.
   *
   * Resumes parent task if we are the last child.
   */
  [[nodiscard]] constexpr auto final_suspend() const noexcept {  // NOLINT
    struct final_awaitable : std::suspend_always {
      // NOLINTNEXTLINE(readability-function-cognitive-complexity)
      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type> child) const noexcept -> raw_handle<> {
        //
        promise_type const& prom = child.promise();

        DEBUG_TRACKER("task reaches final suspend");

        ASSERT_ASSUME(prom.m_context, "execution context not set");
        ASSERT_ASSUME(prom.m_steals == 0, "fork without join");

        if (prom.m_is_inline) {
          DEBUG_TRACKER("inline task resumes parent");

          ASSERT_ASSUME(prom.m_parent, "inline task has no parent");
          ASSERT_ASSUME(prom.m_parent->m_this, "inline task's parents this pointer not set");

          prom.m_parent->m_context = prom.m_context;  // in-case we stole an inline task

          return destroy_if_void(child, prom.m_parent->m_this);
        }

        if (std::optional parent_handle = prom.m_context->pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep rippin!
          DEBUG_TRACKER("fast path, keeps ripping");

          ASSERT_ASSUME(prom.m_parent, "parent is null -> task is root but, pop() non-null");
          ASSERT_ASSUME(parent_handle->m_promise->m_this, "parent's this handle is null");
          ASSERT_ASSUME(prom.m_parent->m_this == parent_handle->m_promise->m_this, "bad parent");
          ASSERT_ASSUME(parent_handle->m_promise->m_context == prom.m_context, "context changed");
          return destroy_if_void(child, parent_handle->m_promise->m_this);
        }

        if (!prom.m_parent) {
          DEBUG_TRACKER("task is parentless and returns");
          return destroy_if_void(child, std::noop_coroutine());
        }

        DEBUG_TRACKER("task's parent was stolen");

        // Register with parent we have completed this child task.
        if (prom.m_parent->m_join.fetch_sub(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent.
          // Hence, we should continue parent, therefore we must set the parents context.

          DEBUG_TRACKER("task is last child to join and resumes parent");

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

  template <typename U>
  [[nodiscard]] constexpr auto await_transform(fork<promise_type<U, Context>> child) noexcept {
    //
    struct awaitable : unique_handle<promise_type<U, Context>> {
      //
      [[nodiscard]] static constexpr auto await_ready() noexcept -> bool { return false; }

      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type> parent) noexcept -> raw_handle<> {
        // In case *this (awaitable) is destructed by stealer after push
        raw_handle<> child = this->promise().m_this;

        DEBUG_TRACKER("task is forking");

        parent.promise().m_context->push(task_handle<Context>{parent.promise()});

        return child;
      }

      [[nodiscard]] constexpr auto await_resume() noexcept -> std::conditional_t<std::is_void_v<T>, void, future<T>> {
        if constexpr (std::is_void_v<T>) {
          // Promise cleaned up at final_suspend.
          DEBUG_TRACKER("releasing void promise");
          this->release();
          return;
        } else {
          return {std::move(*this)};
        }
      }
    };

    child->promise().m_parent = this;
    child->promise().m_context = this->m_context;

    return awaitable{std::move(child)};
  }

  template <typename U>
  [[nodiscard]] constexpr auto await_transform(just<promise_type<U, Context>> child) noexcept {
    //
    struct awaitable : unique_handle<promise_type<U, Context>> {
      //
      [[nodiscard]] static constexpr auto await_ready() noexcept -> bool { return false; }

      [[nodiscard]] constexpr auto await_suspend(raw_handle<promise_type>) noexcept -> raw_handle<> {  // NOLINT
        DEBUG_TRACKER("launching inline task");
        return this->promise().m_this;
      }

      [[nodiscard]] constexpr auto await_resume() noexcept -> std::conditional_t<std::is_void_v<T>, void, T> {
        if constexpr (std::is_void_v<T>) {
          // Promise cleaned up at final_suspend.
          DEBUG_TRACKER("releasing void promise");
          this->release();
          return;
        } else {
          return std::move(this->promise()).get();
        }
      }
    };

    child->promise().m_parent = this;
    child->promise().m_context = this->m_context;

    return awaitable{std::move(child)};
  }

  auto await_transform(join) noexcept {  // NOLINT
    struct awaitable {
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // Currently:            m_join = k_imax - num_joined
        // Hence:       k_imax - m_join = num_joined

        // If no steals then we are the only owner of the parent and we are ready to join.
        if (m_promise->m_steals == 0) {
          DEBUG_TRACKER("sync() ready (no steals)");
          return true;
        }
        // Could use (relaxed) + (fence(acquire) in truthy branch) but, its better if we see all the
        // decrements to m_join and avoid suspending the coroutine if possible.
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

        auto steals = m_promise->m_steals;
        auto joined = m_promise->m_join.fetch_sub(k_imax - steals, std::memory_order_release);

        if (steals == k_imax - joined) {
          // We set n after all children had completed therefore we can resume task.

          // Need to acquire to ensure we see all writes by other threads to the result.
          std::atomic_thread_fence(std::memory_order_acquire);

          DEBUG_TRACKER("sync() wins");
          return task;
        }
        // Someone else is responsible for running this task and we have run out of work.
        DEBUG_TRACKER("sync() looses");
        return std::noop_coroutine();
      }

      constexpr void await_resume() const noexcept {
        // After a sync we reset a/n
        m_promise->m_steals = 0;
        // We know we are the only thread who can touch this promise until a steal which would
        // provide the required memory syncronisation.
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
template <typename T, context Context>
class [[nodiscard]] basic_task : unique_handle<detail::promise_type<T, Context>> {
 public:
  using value_type = T;                                   ///< The type of value that this task will return.
  using future_type = future<T>;                          ///< The type of future that this task must bind to.
  using context_type = Context;                           ///< The type of execution context that this task will run on.
  using promise_type = detail::promise_type<T, Context>;  ///< The type of promise that this task will use.
  using handle_type = task_handle<Context>;               ///< The type of handle that this task will use.

  /**
   * @brief A named tuple returned by ``get_handle()``.
   *
   * In the case of a void task the ``future`` member will be ``regular_void``. This enables
   * consistent structured bindings.
   */
  struct two_tuple {
    [[no_unique_address]] std::conditional<std::is_void_v<T>, regular_void, future_type> future;  ///< Future to result.
    [[no_unique_address]] handle_type handle;                                                     ///< Resumable handle
  };

  /**
   * @brief Decompose this task into a future and a handle.
   *
   * This requires the task to embed a notification mechanism to allow the caller to know when
   * the future is ready.
   */
  [[nodiscard]] constexpr auto get_handle() && noexcept -> two_tuple {
    ASSERT_ASSUME(this, "attempting to get handle from null task");

    handle_type hand{this->promise()};

    if constexpr (std::is_void_v<T>) {
      this->release();
      return {regular_void{}, hand};
    } else {
      return {future_type{std::move(*this)}, hand};
    }
  }

  /**
   * @brief Get an awaitable which will run this task inline.
   */
  [[nodiscard]] constexpr auto just() && noexcept -> detail::just<Context> {
    this->promise().m_is_inline = true;
    return {std::move(*this)};
  }
  /**
   * @brief Get an awaitbale which will cause the current task to fork.
   */
  [[nodiscard]] constexpr auto fork() && noexcept -> detail::fork<Context> { return {std::move(*this)}; }

#ifndef NDEBUG  // Rule of zero in release.
  constexpr ~basic_task() noexcept { ASSERT_ASSUME(!*this, "task destructed without co_await"); }
#endif

 private:
  friend class detail::promise_type<T, Context>;

  constexpr explicit basic_task(detail::raw_handle<promise_type> handle) : unique_handle<promise_type>{handle} {}
};

}  // namespace lf