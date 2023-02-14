#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <coroutine>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>

#include "utility.hpp"

/**
 * @file task.hpp
 *
 * @brief The task class and associated utilities.
 *
 */

namespace lf {

/**
 * @brief An alias for ``std::coroutine_handle<T>`
 */
template <typename T = void>
using coro_h = std::coroutine_handle<T>;

/**
 * @brief Represents a future value to be computed.
 *
 * This is an alias for ``T`` or ``std::optional<T>`` depending on if ``T`` is a trivial type.
 */
template <typename T>
using future = std::conditional_t<std::is_trivial_v<T>, T, std::optional<T>>;

/**
 * @brief A mixin that provides the ``return_[void||value]`` and related methods.
 *
 * General case for non-void types.
 */
template <typename T>
class promise_result {
 public:
  /**
   * @brief Assign ``value`` to the future.
   */
  template <typename U, typename F = future<T>>
  requires std::is_assignable_v<F&, U&&>
  constexpr auto return_value(U&& value) noexcept(std::is_nothrow_assignable_v<F&, U&&>) -> void {
    FORK_LOG("task returns value");
    ASSERT_ASSUME(m_result, "assigning to null future");
    *m_result = std::forward<U>(value);
  }

  /**
   * @brief Set the result ptr for this task.
   */
  constexpr auto set_result_ptr(future<T>& result) -> void { m_result = std::addressof(result); }

 private:
  // private:
  future<T>* m_result = nullptr;  ///< Pointer to the future to assign to.
};

/**
 * @brief A mixin that provides the return related methods.
 *
 * Specialization for void types.
 */
template <>
class promise_result<void> {
 public:
  /**
   * @brief Noop for void returns.
   */
  static constexpr void return_void() noexcept { FORK_LOG("task returns void"); }
};

static constexpr int k_imax = std::numeric_limits<int>::max();  ///< Initial value of ``m_join``.

/**
 * @brief A mixin that provides, initial/final suspend, exception handling and fork-join count.
 */
template <typename Stack>
class promise {
 public:
  /**
   * @brief Construct a new promise base object with a coroutine handle to the derived promise.
   */
  explicit promise(coro_h<> coro) noexcept : m_this{coro} {}
  /**
   * @brief Tasks must be lazy as the parent needs to be pushed onto the stack.
   */
  [[nodiscard]] static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  /**
   * @brief Called when an exception is thrown in the coroutine and not handled.
   *
   * This will store the exception in the parent task. The parent will rethrow the children's
   * exceptions at a join/sync point.
   */
  auto unhandled_exception() const noexcept -> void {
    ASSERT_ASSUME(m_parent, "parent is null, is a root/inline task throwing?");
    if (!m_parent->m_exception_flag.test_and_set()) {
      FORK_LOG("stack unhandled exception");
      ASSERT_ASSUME(!m_parent->m_exception, "parent exception is not null");
      m_parent->m_exception = std::current_exception();
    }
    FORK_LOG("unhandled exception ignored");
  }

  // Intrinsic
  bool m_is_inline = false;  ///< True if this task is running inline.
  coro_h<> m_this;           ///< The coroutine handle for this promise.

  // Extrinsic
  promise* m_parent{};  ///< To promise of task that spawned this task.
  Stack* m_stack{};     ///< Handle to our execution context's stack.

  // State
  int m_steals = 0;                  ///< Number of steals.
  std::atomic<int> m_join = k_imax;  ///< Number of children joined (obfuscated).

  // Exception handling
  std::atomic_flag m_exception_flag = ATOMIC_FLAG_INIT;  ///< True if exception has been set.
  std::exception_ptr m_exception{};                      ///< For child tasks to set.
};

/**
 * @brief Defines the interface for. ..
 *
 * @tparam Stack
 */
template <typename Stack>
concept context = requires(Stack stack, promise<Stack>* prom) {
                    { stack.push(prom) } -> std::same_as<void>;
                    { stack.pop() } -> std::convertible_to<promise<Stack>*>;
                  };

/**
 * @brief Destroy a non-null handle.
 *
 * This is utility function that writes allows you to store a copy of to_forward before destroying a
 * promise that may be storing a continuation.
 *
 * @param to_destroy Handle to destroy
 * @param to_forward Handle to forward/return
 * @return coro_h<P2> A copy of ``to_forward``
 */
template <typename P1, typename P2 = void>
inline auto destroy(coro_h<P1> to_destroy, coro_h<P2> to_forward = nullptr) noexcept -> coro_h<P2> {
  FORK_LOG("call to destroy()");
  ASSERT_ASSUME(to_destroy, "attempting to destroy null handle");
  to_destroy.destroy();
  return to_forward;
}

namespace tag {

/**
 * @brief Tag type to signify task should fork.
 */
template <typename Stack>
struct fork {
  promise<Stack>* m_promise;  ///< Handle to the promise to fork.
};
/**
 * @brief Tag type to signify task should join.
 */
struct join {};
/**
 * @brief Tag type to signify a task should be run inline.
 */
template <typename Stack>
struct just {
  promise<Stack>* m_promise;  ///< Handle to the promise to run inline.
};

}  // namespace tag

/**
 * @brief Utility ``co_await`` on to join a forked task.
 */
inline constexpr tag::join join = {};

/**
 * @brief ...
 *
 * @tparam T
 * @tparam Stack
 */
template <typename T, typename Stack>
class [[nodiscard("a task will leak unless it is run to final_suspend")]] task {
 public:
  using value_type = T;  ///< The type of value that this task will return.

  struct promise_type;

  //  private:

  /**
   * @brief Construct a new task object, only the ``promise_type`` can access this
   */
  constexpr explicit task(promise_type * promise) noexcept : m_promise{promise} {
    FORK_LOG("task is constructed");
  }
  /**
   * @brief Fork a task.
   */
  friend auto fork(future<T> & fut, task tsk) noexcept -> tag::fork<Stack>
  requires(!std::is_void_v<T>)
  {
    tsk.m_promise->set_result_ptr(fut);
    return {tsk.m_promise};
  }
  /**
   * @brief Fork a task.
   */
  friend auto fork(task tsk) noexcept -> tag::fork<Stack>
  requires std::is_void_v<T>
  {
    return {tsk.m_promise};
  }
  /**
   * @brief Run a task inline.
   */
  friend auto just(future<T> & fut, task tsk) noexcept -> tag::just<Stack>
  requires(!std::is_void_v<T>)
  {
    tsk.m_promise->set_result_ptr(fut);
    tsk.m_promise->m_is_inline = true;
    return {tsk.m_promise};
  }
  /**
   * @brief Run a task inline.
   */
  friend auto just(task tsk) noexcept -> tag::just<Stack>
  requires std::is_void_v<T>
  {
    tsk.m_promise->m_is_inline = true;
    return {tsk.m_promise};
  }

  //  private

  promise_type* m_promise;  ///< The promise for this task.
};

/**
 * @brief The promise type for a task.
 */
template <typename T, typename Stack>
struct task<T, Stack>::promise_type : promise<Stack>, promise_result<T> {
  /**
   * @brief Construct a new promise type object.
   */
  constexpr promise_type() noexcept : promise<Stack>{coro_h<promise_type>::from_promise(*this)} {}
  /**
   * @brief This is the object returned when a task is created by a function call.
   */
  constexpr auto get_return_object() noexcept -> task { return task{this}; }

  /**
   * @brief Called at end of coroutine frame.
   *
   * Resumes parent task if we are the last child.
   */
  [[nodiscard]] auto final_suspend() const noexcept {  // NOLINT
    struct final_awaitable : std::suspend_always {
      // NOLINTNEXTLINE(readability-function-cognitive-complexity)
      [[nodiscard]] auto await_suspend(coro_h<promise_type> handle) const noexcept -> coro_h<> {
        //
        promise_type const& prom = handle.promise();

        FORK_LOG("task reaches final suspend");

        ASSERT_ASSUME(prom.m_stack, "execution context not set");

        if (prom.m_is_inline) {
          FORK_LOG("inline task resumes parent");
          ASSERT_ASSUME(prom.m_parent, "inline task has no parent");
          ASSERT_ASSUME(prom.m_parent->m_this, "inline task's parents this pointer not set");
          return destroy(handle, prom.m_parent->m_this);
        }

        if (promise<Stack>* parent_handle = prom.m_stack->pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep rippin!
          FORK_LOG("fast path, keeps ripping");

          ASSERT_ASSUME(prom.m_parent, "parent is null -> task is root but, pop() non-null");
          ASSERT_ASSUME(parent_handle->m_this, "parent's this handle is null");
          ASSERT_ASSUME(prom.m_parent->m_this == parent_handle->m_this, "pop() is not parent");
          ASSERT_ASSUME(parent_handle->m_stack == prom.m_stack, "stack changed");

          return destroy(handle, parent_handle->m_this);  // Destrory the child, resume parent.
        }

        if (!prom.m_parent) {
          FORK_LOG("task is parentless and returns");
          return destroy(handle, std::noop_coroutine());
        }

        FORK_LOG("task's parent was stolen");

        // Register with parent we have completed this child task.
        auto children_to_join = prom.m_parent->m_join.fetch_sub(1, std::memory_order_release);

        if (children_to_join == 1) {
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent.
          // Hence, we should continue parent, therefore we must set the parents context.

          FORK_LOG("task is last child to join and resumes parent");

          ASSERT_ASSUME(prom.m_parent->m_this, "parent's this handle is null");
          ASSERT_ASSUME(prom.m_parent->m_stack != prom.m_stack, "parent has same context");

          prom.m_parent->m_stack = prom.m_stack;

          return destroy(handle, prom.m_parent->m_this);
        }
        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, yield to executor.
        FORK_LOG("task is not last to join");
        return destroy(handle, std::noop_coroutine());
      }
    };
    return final_awaitable{};
  }

  /**
   * @brief todo
   *
   * @tparam U
   * @param child
   * @return auto
   */
  auto await_transform(tag::fork<Stack> child) noexcept {
    //
    struct fork_awaitable : coro_h<>, std::suspend_always {
      //
      auto await_suspend(coro_h<promise_type> parent) -> coro_h<> {
        // In case *this (awaitable) is destructed by stealer after push
        coro_h<> child = *this;

        FORK_LOG("task is forking");

        parent.promise().m_stack->push(&parent.promise());

        return child;
      }
    };

    child.m_promise->m_parent = this;
    child.m_promise->m_stack = this->m_stack;

    return fork_awaitable{child.m_promise->m_this, {}};
  }

  /**
   * @brief TODO
   *
   * @param child
   * @return auto
   */
  auto await_transform(tag::just<Stack> child) noexcept {
    //
    struct inline_awaitable : coro_h<>, std::suspend_always {
      //
      auto await_suspend(coro_h<promise_type>) -> coro_h<> {
        FORK_LOG("launching inline task");
        return *this;
      }
    };

    child.m_promise->m_parent = this;
    child.m_promise->m_stack = this->m_stack;

    return inline_awaitable{child.m_promise->m_this, {}};
  }

  /**
   * @brief todo
   *
   */
  auto await_transform(tag::join) noexcept {  // NOLINT
    struct awaitable {
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // Currently:            m_join = k_imax - num_joined
        // Hence:       k_imax - m_join = num_joined

        // If no steals then we are the only owner of the parent and we are ready to join.
        if (m_promise->m_steals == 0) {
          FORK_LOG("sync() ready (no steals)");
          return true;
        }
        // Could use (relaxed) + (fence(acquire) in truthy branch) but, its better if we see all the
        // decrements to m_join and avoid suspending the coroutine if possible.
        if (m_promise->m_steals == k_imax - m_promise->m_join.load(std::memory_order_acquire)) {
          FORK_LOG("sync() is ready");
          return true;
        }
        FORK_LOG("sync() not ready");
        return false;
      }

      auto await_suspend(coro_h<promise_type> task) noexcept -> coro_h<> {
        // Currently        m_join = k_imax - num_joined
        // We set           m_join = m_join - (k_imax - num_steals)
        //                         = num_steals - num_joined
        auto steals = m_promise->m_steals;
        auto joined = m_promise->m_join.fetch_sub(k_imax - steals, std::memory_order_release);
        // Hence            joined = k_imax - num_joined
        //         k_imax - joined = num_joined
        if (steals == k_imax - joined) {
          // We set n after all children had completed therefore we can resume task.

          // Need to acquire to ensure we see all writes by other threads to the result.
          std::atomic_thread_fence(std::memory_order_acquire);

          FORK_LOG("sync() wins");
          return task;
        }
        // Someone else is responsible for running this task and we have run out of work.
        FORK_LOG("sync() looses");
        return std::noop_coroutine();
      }

      constexpr void await_resume() const noexcept {
        // After a sync we reset a/n
        m_promise->m_steals = 0;
        // We know we are the only thread who can touch this promise until a steal which whould
        // provide the required memory syncronisation.
        m_promise->m_join.store(k_imax, std::memory_order_relaxed);
      }

      promise<Stack>* m_promise;
    };

    return awaitable{this};
  }

  ~promise_type() { FORK_LOG("task is destructed"); }
};

}  // namespace lf