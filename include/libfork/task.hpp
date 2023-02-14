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

 private:
  future<T>* m_result = nullptr;
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

/**
 * @brief A mixin that provides, initial/final suspend, exception handling and fork-join count.
 */
template <typename Stack>
struct promise {
  /**
   * @brief Construct a new promise base object with a coroutine handle to the derived promise.
   */
  explicit promise(coro_h<> coro) noexcept : m_this{coro} {}
  /**
   * @brief Tasks must be lazy as the parent needs to be pushed onto the stack.
   */
  [[nodiscard]] static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  /**
   * @brief On termination return control to the caller.
   */
  [[nodiscard]] static auto final_suspend() noexcept -> std::suspend_never { return {}; }

  /**
   * @brief Called when an exception is thrown in the coroutine and not handled.
   *
   * This will store the exception in the parent task. The parent will rethrow the children's
   * exceptions at a join/sync point.
   */
  auto unhandled_exception() const noexcept -> void {
    ASSERT_ASSUME(m_parent, "parent is null, is a root task throwing?");
    if (!m_parent->m_exception_flag.test_and_set()) {
      FORK_LOG("stack unhandled exception");
      ASSERT_ASSUME(!m_parent->m_exception, "parent exception is not null");
      m_parent->m_exception = std::current_exception();
    }
    FORK_LOG("unhandled exception ignored");
  }

  // Constants
  static constexpr int k_imax = std::numeric_limits<int>::max();  ///< Initial value of ``m_join``.

  // Intrinsic
  coro_h<> m_this;  ///< The coroutine handle for this promise.

  // Extrinsic
  promise* m_parent{};  ///< To promise of task that spawned this task.
  Stack* m_stack{};     ///< Handle to our execution context's stack.

  // State
  int m_alpha = 0;                   ///< Number of steals.
  std::atomic<int> m_join = k_imax;  ///< Number of children joined (obfuscated).

  // Exception handling
  std::atomic_flag m_exception_flag = ATOMIC_FLAG_INIT;  ///< True if exception has been set.
  std::exception_ptr m_exception;                        ///< For child tasks to set.
};

/**
 * @brief Defines the interface for. ..
 *
 * @tparam Stack
 */
template <typename Stack>
concept context = requires(Stack stack, promise<Stack>* promise) {
                    { stack.push(promise) } -> std::same_as<void>;
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
  FORK_LOG("destroying a task");
  ASSERT_ASSUME(to_destroy, "attempting to destroy null handle");
  to_destroy.destroy();
  return to_forward;
}

// template <typename T, typename Stack>
// class task;

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

  /**
   * @brief The promise type for this task.
   */
  struct promise_type : promise<Stack>, promise_result<T> {
    /**
     * @brief Construct a new promise type object.
     */
    constexpr promise_type() noexcept : promise<Stack>{coro_h<promise_type>::from_promise(*this)} {}
    /**
     * @brief This is the object returned when a task is created by a function call.
     */
    constexpr auto get_return_object() noexcept -> task { return task{this}; }

    ~promise_type() { FORK_LOG("task is destructed"); }
  };

  //  private:

  /**
   * @brief Construct a new task object, only the ``promise_type`` can access this
   */
  constexpr explicit task(promise_type * promise) noexcept : m_promise{promise} {}

  promise_type* m_promise;  ///< The promise for this task.
};

}  // namespace lf