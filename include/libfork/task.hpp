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

#include "utility.hpp"

/**
 * @file task.hpp
 *
 * @brief The task class and associated utilities.
 *
 */

namespace lf {

/**
 * @brief Represents a future value to be computed.
 *
 * This is an alias for ``T`` or ``std::optional<T>`` depending on if ``T`` is a trivial type.
 *
 * \rst
 *
 * .. warning::
 *    A future must **not** be copied or moved once a task has been forked.
 *
 * \endrst
 */
template <typename T>
using future = std::conditional_t<std::is_trivial_v<T>, T, std::optional<T>>;

/**
 * @brief An alias for ``std::coroutine_handle<T>`
 */
template <typename T = void>
using handle = std::coroutine_handle<T>;

namespace detail {

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
  explicit promise_base(handle<> coro) noexcept : m_this{coro} {}
  /**
   * @brief Tasks must be lazy as the parent needs to be pushed onto the stack.
   */
  [[nodiscard]] static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  /**
   * @brief Called when an exception is thrown in the coroutine and not handled.
   *
   * This will crash the program.
   */
  [[noreturn]] auto unhandled_exception() const noexcept -> void {
    DEBUG_TRACKER("unhandled_exception");
    std::terminate();
  }

  // Intrinsic
  bool m_is_inline = false;  ///< True if this task is running inline.
  handle<> m_this;           ///< The coroutine handle for this promise.

  // Extrinsic
  promise_base* m_parent{};  ///< To promise of task that spawned this task.
  Context* m_stack{};        ///< Context to our execution context's stack.

  // State
  int m_steals = 0;                  ///< Number of steals.
  std::atomic<int> m_join = k_imax;  ///< Number of children joined (obfuscated).
};

/**
 * @brief A specialisation that provides the ``return_value()`` and related methods.
 */
template <typename T, typename Context>
struct promise_result : promise_base<Context> {
  /**
   * @brief Inherit constructors from ``promise_base``.
   */
  using promise_base<Context>::promise_base;

  /**
   * @brief Assign ``value`` to the future.
   */
  template <typename U, typename F = future<T>>
  requires std::is_assignable_v<F&, U&&>
  auto return_value(U&& value) noexcept(std::is_nothrow_assignable_v<F&, U&&>) -> void {
    DEBUG_TRACKER("task returns value");
    ASSERT_ASSUME(m_result, "assigning to null future");
    *m_result = std::forward<U>(value);
  }

  /**
   * @brief Set the return address for this task.
   */
  auto set_result_ptr(future<T>& result) noexcept -> void { m_result = std::addressof(result); }

 private:
  future<T>* m_result = nullptr;
};

/**
 * @brief A specialisation that provides ``return_void()``.
 */
template <typename Context>
struct promise_result<void, Context> : promise_base<Context> {
  /**
   * @brief Inherit constructors from ``promise_base``.
   */
  using promise_base<Context>::promise_base;
  /**
   * @brief Noop for void returns.
   */
  static void return_void() noexcept { DEBUG_TRACKER("task returns void"); }
};

// Tag types.

template <typename Context>
struct fork {
  promise_base<Context>* m_promise;
};

struct join {};

template <typename Context>
struct just {
  promise_base<Context>* m_promise;
};

/**
 * @brief The promise type for a task.
 */
template <typename T, typename Context>
struct promise_type : promise_result<T, Context> {
  /**
   * @brief Construct a new promise type object.
   */
  promise_type() noexcept : promise_result<T, Context>{handle<promise_type>::from_promise(*this)} {}
  /**
   * @brief This is the object returned when a task is created by a function call.
   */
  auto get_return_object() noexcept -> promise_type* { return this; }

  /**
   * @brief Called at end of coroutine frame.
   *
   * Resumes parent task if we are the last child.
   */
  [[nodiscard]] auto final_suspend() const noexcept {  // NOLINT
    struct final_awaitable : std::suspend_always {
      // NOLINTNEXTLINE(readability-function-cognitive-complexity)
      [[nodiscard]] auto await_suspend(handle<promise_type> child) const noexcept -> handle<> {
        //
        promise_type const& prom = child.promise();

        DEBUG_TRACKER("task reaches final suspend");

        ASSERT_ASSUME(prom.m_stack, "execution context not set");
        ASSERT_ASSUME(prom.m_steals == 0, "fork without join");

        if (prom.m_is_inline) {
          DEBUG_TRACKER("inline task resumes parent");

          ASSERT_ASSUME(prom.m_parent, "inline task has no parent");
          ASSERT_ASSUME(prom.m_parent->m_this, "inline task's parents this pointer not set");
          return destroy(child, prom.m_parent->m_this);
        }

        if (std::optional parent_handle = prom.m_stack->pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep rippin!
          DEBUG_TRACKER("fast path, keeps ripping");

          ASSERT_ASSUME(prom.m_parent, "parent is null -> task is root but, pop() non-null");
          ASSERT_ASSUME(parent_handle->m_promise->m_this, "parent's this handle is null");
          ASSERT_ASSUME(prom.m_parent->m_this == parent_handle->m_promise->m_this, "bad parent");
          ASSERT_ASSUME(parent_handle->m_promise->m_stack == prom.m_stack, "stack changed");
          return destroy(child, parent_handle->m_promise->m_this);
        }

        if (!prom.m_parent) {
          DEBUG_TRACKER("task is parentless and returns");
          return destroy(child, std::noop_coroutine());
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
          ASSERT_ASSUME(prom.m_parent->m_stack != prom.m_stack, "parent has same context");

          prom.m_parent->m_stack = prom.m_stack;

          return destroy(child, prom.m_parent->m_this);
        }
        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, yield to executor.
        DEBUG_TRACKER("task is not last to join");
        return destroy(child, std::noop_coroutine());
      }
    };
    return final_awaitable{};
  }

  auto await_transform(fork<Context> child) noexcept {
    //
    struct fork_awaitable : handle<>, std::suspend_always {
      //
      auto await_suspend(handle<promise_type> parent) -> handle<> {
        // In case *this (awaitable) is destructed by stealer after push
        handle<> child = *this;

        DEBUG_TRACKER("task is forking");

        parent.promise().m_stack->push({parent.promise()});

        return child;
      }
    };

    child.m_promise->m_parent = this;
    child.m_promise->m_stack = this->m_stack;

    return fork_awaitable{child.m_promise->m_this, {}};
  }

  auto await_transform(just<Context> child) noexcept {
    //
    struct inline_awaitable : handle<>, std::suspend_always {
      //
      auto await_suspend(handle<promise_type>) -> handle<> {  // NOLINT
        DEBUG_TRACKER("launching inline task");
        return *this;
      }
    };

    child.m_promise->m_parent = this;
    child.m_promise->m_stack = this->m_stack;

    return inline_awaitable{child.m_promise->m_this, {}};
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

      auto await_suspend(handle<promise_type> task) noexcept -> handle<> {
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
        // We know we are the only thread who can touch this promise until a steal which whould
        // provide the required memory syncronisation.
        m_promise->m_join.store(k_imax, std::memory_order_relaxed);
      }

      promise_base<Context>* m_promise;
    };

    return awaitable{this};
  }

 private:
  /**
   * @brief Destroy a non-null handle.
   *
   * This is utility function that writes allows you to store a copy of to_forward before destroying
   * a promise that may be storing a continuation.
   *
   * @param to_destroy Context to destroy
   * @param to_forward Context to forward/return
   * @return handle<P2> A copy of ``to_forward``
   */
  template <typename P1, typename P2 = void>
  static inline auto destroy(handle<P1> to_destroy, handle<P2> to_forward) noexcept -> handle<P2> {
    DEBUG_TRACKER("call to destroy()");
    ASSERT_ASSUME(to_destroy, "attempting to destroy null handle");
    to_destroy.destroy();
    return to_forward;
  }
};

}  // namespace detail

/**
 * @brief A handle to a task.
 *
 * @tparam Context The handle type of the execution context that this task is running on.
 */
template <typename Context>
class task_handle {
 public:
  /**
   * @brief To make task_handle trivial.
   */
  task_handle() = default;

  /**
   * @brief Construct a handle to a promise.
   */
  // NOLINTNEXTLINE (non explicit is required)
  constexpr task_handle(detail::promise_base<Context>& coro) noexcept : m_promise{&coro} {}

  /**
   * @brief Resume the coroutine associated with this handle.
   *
   * This should be called by the thread owning the execution context that this task is running
   * on.
   */
  void resume() const noexcept {
    ASSERT_ASSUME(m_promise, "resuming null handle");
    ASSERT_ASSUME(m_promise->m_this, "resuming null coroutine");
    m_promise->m_this.resume();
  }

  /**
   * @brief Resume the coroutine associated with this handle.
   *
   * This should be called on a handle stolen from another context.
   */
  void resume_stolen(Context& context) const noexcept {
    ASSERT_ASSUME(m_promise, "resuming null handle");
    ASSERT_ASSUME(m_promise->m_stack, "resuming stolen handle with null stack");
    ASSERT_ASSUME(m_promise->m_stack != std::addressof(context), "bad steal call");
    ASSERT_ASSUME(m_promise->m_this, "resuming null coroutine");
    m_promise->m_stack = std::addressof(context);
    m_promise->m_steals += 1;
    m_promise->m_this.resume();
  }

 private:
  template <typename, typename>
  friend class detail::promise_type;

  detail::promise_base<Context>* m_promise;
};

/**
 * @brief Defines the interface for an execution context.
 *
 * \rst
 *
 * Specifically:
 *
 * .. include:: ../../include/libfork/task.hpp
 *    :code:
 *    :start-line: 213
 *    :end-before: //! END-CONTEXT-CAPTURE
 *
 * \endrst
 */
template <typename Context>
concept context = requires(Context stack, task_handle<Context> task) {
                    { stack.push(task) } -> std::same_as<void>;
                    { stack.pop() } -> std::same_as<std::optional<task_handle<Context>>>;
                  };
//! END-CONTEXT-CAPTURE

/**
 * @brief ...
 *
 * @tparam T
 * @tparam Context
 */
template <typename T, typename Context>
class [[nodiscard("a task will leak unless it is run to final_suspend")]] task {
 public:
  using value_type = T;           ///< The type of value that this task will return.
  using future_type = future<T>;  ///< The type of future that this task must bind to.
  using context_type = Context;   ///< The type of execution context that this task will run on.
  using promise_type = detail::promise_type<T, Context>;  ///< The type of promise that this task
                                                          ///< will use.

  /**
   * @brief Construct a new task object, only the ``promise_type`` can access this
   */
  constexpr task(promise_type * promise) noexcept : m_promise{promise} {  // NOLINT
    ASSERT_ASSUME(promise, "promise is null");
    DEBUG_TRACKER("task is constructed");
  }

 private:
  [[nodiscard]] friend auto sync_wait(Context & context, task && tsk) noexcept -> T {
    //

    std::binary_semaphore sem{0};

    future<T> res;

    task root_task = [](task&& tsk, std::binary_semaphore& sem) -> task {
      //
      defer at_exit{[&sem]() noexcept {
        DEBUG_TRACKER("semaphore released");
        sem.release();
      }};

      future<T> res;

      co_await just(res, std::move(tsk));

      co_return res;
    }(std::move(tsk), sem);

    root_task.m_promise->set_result_ptr(res);
    root_task.m_promise->m_stack = std::addressof(context);

    context.submit(task_handle<Context>{*root_task.m_promise});

    sem.acquire();
    DEBUG_TRACKER("semaphore acquired");

    return res;
  }

  friend auto just(future_type & fut, task && tsk) noexcept -> detail::just<Context>
  requires(!std::is_void_v<T>)
  {
    tsk.m_promise->set_result_ptr(fut);
    tsk.m_promise->m_is_inline = true;
    return {tsk.m_promise};
  }

  friend auto just(task && tsk) noexcept -> detail::just<Context>
  requires std::is_void_v<T>
  {
    tsk.m_promise->m_is_inline = true;
    return {tsk.m_promise};
  }

  friend auto fork(future_type & fut, task && tsk) noexcept -> detail::fork<Context>
  requires(!std::is_void_v<T>)
  {
    tsk.m_promise->set_result_ptr(fut);
    return {tsk.m_promise};
  }

  friend auto fork(task && tsk) noexcept -> detail::fork<Context>
  requires std::is_void_v<T>
  {
    return {tsk.m_promise};
  }

  promise_type* m_promise;  ///< The promise for this task.
};

namespace detail {

template <typename, typename>
struct is_task_for : std::false_type {};

template <typename T, context Context>
struct is_task_for<future<T>, task<T, Context>> : std::true_type {};

template <context Context>
struct is_task_for<void, task<void, Context>> : std::true_type {};

template <typename T, typename Task>
inline constexpr bool is_task_for_v = is_task_for<T, Task>::value;

}  // namespace detail

/**
 * @brief Build a task and get an awaitable to run it inline.
 *
 * @param fut
 * @param fun
 * @param args
 * @return requires
 */
template <typename F, typename... Args, std::invocable<Args...> Fn>
requires detail::is_task_for_v<F, std::invoke_result_t<Fn, Args...>>
auto just(F& fut, Fn&& fun, Args&&... args) {
  return just(fut, std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...));
}

/**
 * @brief Build a task and get an awaitable to run it inline.
 *
 * @param fun
 * @param args
 * @return requires
 */
template <typename F, typename... Args, std::invocable<Args...> Fn>
requires detail::is_task_for_v<void, std::invoke_result_t<Fn, Args...>>
auto just(Fn&& fun, Args&&... args) {
  return just(std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...));
}

/**
 * @brief Build a task and get an awaitable to run it inline.
 *
 * @param fut
 * @param fun
 * @param args
 * @return requires
 */
template <typename F, typename... Args, std::invocable<Args...> Fn>
requires detail::is_task_for_v<F, std::invoke_result_t<Fn, Args...>>
auto fork(F& fut, Fn&& fun, Args&&... args) {
  return fork(fut, std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...));
}

/**
 * @brief Build a task and get an awaitable to run it inline.
 *
 * @param fun
 * @param args
 * @return requires
 */
template <typename F, typename... Args, std::invocable<Args...> Fn>
requires detail::is_task_for_v<void, std::invoke_result_t<Fn, Args...>>
auto fork(Fn&& fun, Args&&... args) {
  return fork(std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...));
}

/**
 * @brief Utility,``co_await`` on the result join a forked task.
 */
inline constexpr auto join() noexcept -> detail::join {
  return {};
}

}  // namespace lf