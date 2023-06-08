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

// clang-format off

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to a wrapped task.
 */
template<template<typename ...> typename Packet>
struct bind_task {
  /**
   * @brief Bind return address `ret` to task.
   * 
   * @return A functor, that will return an awaitable, that will trigger a fork/call .
   */
  template <typename R, typename F>
#ifdef __cpp_static_call_operator
  [[nodiscard]] static constexpr auto operator()(R &ret, wrap_fn<F>) noexcept {
#else
  [[nodiscard]] constexpr auto operator()(R &ret, wrap_fn<F>) const noexcept {
#endif
    return [&] <typename... Args>(Args &&...args) noexcept -> Packet<R, F, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }

  /**
   * @brief Set a void return address for a task.
   * 
   * @return A functor, that will return an awaitable, that will trigger a fork/call .
   */
  template <typename F>
#ifdef __cpp_static_call_operator
  [[nodiscard]] static constexpr auto operator()(wrap_fn<F>) noexcept {
#else
  [[nodiscard]] constexpr auto operator()(wrap_fn<F>) const noexcept {
#endif
    return [&] <typename... Args>(Args &&...args) noexcept -> Packet<void, F, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }

#if defined(LIBFORK_DOXYGEN_SHOULD_SKIP_THIS) || defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to task.
   * 
   * @return A functor, that will return an awaitable, that will trigger a fork/call .
   */
  template <typename R, typename F>
   [[nodiscard]] static constexpr auto operator[](R &ret, wrap_fn<F> func) noexcept {
    return bind_task{}(ret, func);
  }
  
  /**
   * @brief Set a void return address for a task.
   * 
   * @return A functor, that will return an awaitable, that will trigger a fork/call .
   */
  template <typename F>
   [[nodiscard]] static constexpr auto operator[](wrap_fn<F> func) noexcept {
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
