#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/coroutine.hpp"
#include "libfork/promise.hpp"
#include "libfork/task.hpp"

/**
 * @file libfork.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` and ``lf::sync_wait`` machinery.
 */

#ifndef LIBFORK_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <typename T, typename Context, typename TagWith, typename... Args>
  requires std::same_as<std::remove_cvref_t<TagWith>, TagWith> && lf::detail::tag<typename TagWith::tag>
struct lf::stdexp::coroutine_traits<lf::task<T, Context>, TagWith, Args...> {
  //
  using promise_type = lf::detail::promise_type<T, Context, typename TagWith::tag>;

  #ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(std::is_pointer_interconvertible_with_class<promise_type, lf::detail::control_block_t>(&promise_type::control_block));
  static_assert(std::is_pointer_interconvertible_base_of_v<lf::detail::promise_base<T>, promise_type>);
  #endif
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <typename T, typename Context, typename This, typename TagWith, typename... Args>
  requires std::same_as<std::remove_cvref_t<TagWith>, TagWith> && lf::detail::tag<typename TagWith::tag>
struct lf::stdexp::coroutine_traits<lf::task<T, Context>, This, TagWith, Args...> : lf::stdexp::coroutine_traits<lf::task<T, Context>, TagWith, Args...> {
};

#endif /* LIBFORK_DOXYGEN_SHOULD_SKIP_THIS */

namespace lf {

namespace detail {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T, thread_context Context>
struct is_task_impl<task<T, Context>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

template <typename F, typename... Args>
using invoke_t = std::invoke_result_t<F, detail::magic<detail::root_t, async_fn<F>>, Args...>;

} // namespace detail

/**
 * @brief The entry point for syncronous execution of asyncronous functions.
 *
 * This will create the coroutine and pass its handle to ``schedule``. ``schedule`` is expected to garantee that
 * some thread will call ``resume()`` on the coroutine handle it is passed. The calling thread will then block
 * until the asyncronous function has finished executing. Finally the result of the asyncronous function
 * will be returned.
 */
template <std::invocable<stdexp::coroutine_handle<>> Schedule, stateless F, class... Args, detail::is_task Task = detail::invoke_t<F, Args...>>
  requires std::default_initializable<typename Task::value_type> || std::is_void_v<typename Task::value_type>
auto sync_wait(Schedule &&schedule, async_fn<F>, Args &&...args) -> typename Task::value_type {

  using value_type = typename Task::value_type;

  task const coro = std::invoke(F{}, detail::magic<detail::root_t, async_fn<F>>{}, std::forward<Args>(args)...);

  detail::root_block_t root_block;

  coro.promise().control_block.set(root_block);

  [[maybe_unused]] std::conditional_t<std::is_void_v<value_type>, int, value_type> result;

  if constexpr (!std::is_void_v<value_type>) {
    coro.promise().set_return_address(result);
  }

#if LIBFORK_COMPILER_EXCEPTIONS
  try {
    std::invoke(std::forward<Schedule>(schedule), stdexp::coroutine_handle<>{coro});
  } catch (...) {
    // We cannot know whether the coroutine has been resumed or not once we pass to schedule(...).
    // Hence, we do not know whether or not to .destroy() it if schedule(...) throws.
    // Hence we mark noexcept to trigger termination.
    []() noexcept {
      throw;
    }();
  }
#else
  std::invoke(std::forward<Schedule>(schedule), stdexp::coroutine_handle<>{coro});
#endif

  // Block until the coroutine has finished.
  root_block.acquire();

  root_block.rethrow_if_unhandled();

  if constexpr (!std::is_void_v<value_type>) {
    return result;
  }
}

// clang-format off

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asyncronous function.
 */
template<template<typename ...> typename Packet>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asyncronous function.
   * 
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
#ifdef __cpp_static_call_operator
  [[nodiscard]] static constexpr auto operator()(R &ret, async_fn<F>) noexcept {
#else
  [[nodiscard]] constexpr auto operator()(R &ret, async_fn<F>) const noexcept {
#endif
    return [&] <typename... Args>(Args &&...args) noexcept -> Packet<R, F, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }

  /**
   * @brief Set a void return address for an asyncronous function.
   * 
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
#ifdef __cpp_static_call_operator
  [[nodiscard]] static constexpr auto operator()(async_fn<F>) noexcept {
#else
  [[nodiscard]] constexpr auto operator()(async_fn<F>) const noexcept {
#endif
    return [&] <typename... Args>(Args &&...args) noexcept -> Packet<void, F, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }

#if defined(LIBFORK_DOXYGEN_SHOULD_SKIP_THIS) || defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asyncronous function.
   * 
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
   [[nodiscard]] static constexpr auto operator[](R &ret, async_fn<F> func) noexcept {
    return bind_task{}(ret, func);
  }
  
  /**
   * @brief Set a void return address for an asyncronous function.
   * 
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
   [[nodiscard]] static constexpr auto operator[](async_fn<F> func) noexcept {
    return bind_task{}(func);
  }
#endif
};

// clang-format on

/**
 * @brief An awaitable (in a task) that triggers a join.
 */
inline constexpr detail::join_t join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 */
inline constexpr bind_task<detail::fork_packet> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr bind_task<detail::call_packet> call = {};

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
