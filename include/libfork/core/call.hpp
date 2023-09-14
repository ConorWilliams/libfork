#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>
#include <utility>

#include "libfork/core/task.hpp"

/**
 * @file call.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` and
 * ``lf::sync_wait`` machinery.
 */

namespace lf {

namespace detail {

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
    requires(Tag != tag::tail)
  [[nodiscard("HOF needs to be called")]] LF_STATIC_CALL constexpr auto
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
  [[nodiscard("HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()([[maybe_unused]] async<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }

#if defined(LF_DOXYGEN_SHOULD_SKIP_THIS) ||                                                                                 \
    (defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L)
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
    requires(Tag != tag::tail)
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

struct join_type {};

} // namespace detail

/**
 * @brief An awaitable (in a task) that triggers a join.
 */
inline constexpr detail::join_type join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 */
inline constexpr detail::bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr detail::bind_task<tag::call> call = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a tail-call.
 */
inline constexpr detail::bind_task<tag::tail> tail = {};

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
