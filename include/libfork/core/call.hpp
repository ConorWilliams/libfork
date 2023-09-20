#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core/task.hpp"

/**
 * @file call.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` machinery.
 */

namespace lf {

namespace impl {

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  #define LF_DEPRECATE [[deprecated("Use operator[] instead")]]
#else
  #define LF_DEPRECATE
#endif

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asynchronous function.
 */
template <tag Tag>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
    requires (Tag != tag::tail)
  LF_DEPRECATE [[nodiscard("HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()(R &ret, [[maybe_unused]] async<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<R, Tag, F>, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  LF_DEPRECATE [[nodiscard("HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()([[maybe_unused]] async<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
    requires (Tag != tag::tail)
  [[nodiscard("HOF needs to be called")]] static constexpr auto operator[](R &ret,
                                                                           [[maybe_unused]] async<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<R, Tag, F>, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard("HOF needs to be called")]] static constexpr auto operator[]([[maybe_unused]] async<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }
#endif
};

#undef LF_DEPRECATE

/**
 * @brief A empty tag type used to disambiguate a join.
 */
struct join_type {};

} // namespace impl

inline namespace core {

/**
 * @brief An awaitable (in a `lf::task`) that triggers a join.
 *
 * After a join is resumed it is guaranteed that all forked child tasks will have completed.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no relationship between the thread that executes the ``lf::join`` and the thread that resumes the coroutine.
 *
 * \endrst
 */
inline constexpr impl::join_type join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 *
 * Conceptually the forked/child task can be executed anywhere at anytime and and in parallel with its continuation.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no guaranteed relationship between the thread that executes the ``lf::fork`` and the thread(s) that execute
 *    the continuation/child. However, currently ``libfork`` uses continuation stealing so the thread that calls ``lf::fork``
 *    will immediately begin executing the child.
 *
 * \endrst
 */
inline constexpr impl::bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 *
 * Conceptually the called/child task can be executed anywhere at anytime but, its continuation is guaranteed to be sequenced
 * after the child returns.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no relationship between the thread that executes the ``lf::call`` and the thread(s) that execute the
 *    continuation/child. However, currently ``libfork`` uses continuation stealing so the thread that calls ``lf::call``
 *    will immediately begin executing the child.
 *
 * \endrst
 */
inline constexpr impl::bind_task<tag::call> call = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a
 * [tail-call](https://en.wikipedia.org/wiki/Tail_call).
 */
inline constexpr impl::bind_task<tag::tail> tail = {};

} // namespace core

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
