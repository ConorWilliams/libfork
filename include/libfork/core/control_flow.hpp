
#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility> // for move
#include <version> // for __cpp_multidimensional_subscript

#include "libfork/core/first_arg.hpp"      // for async_function_object, quasi_pointer
#include "libfork/core/impl/combinate.hpp" // for combinate
#include "libfork/core/invocable.hpp"      // for discard_t
#include "libfork/core/macro.hpp"          // for LF_STATIC_CALL, LF_STATIC_CONST, LF_DEPRECATE...
#include "libfork/core/tag.hpp"            // for tag

/**
 * @file control_flow.hpp
 *
 * @brief Meta header which includes ``lf::fork``, ``lf::call``, ``lf::join`` machinery.
 */

namespace lf {

namespace impl {

/**
 * @brief A empty tag type used to disambiguate a join.
 */
struct join_type {};

/**
 * @brief A empty tag type that forces a rethrow of an exception.
 */
struct rethrow_if_exception_type {};

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
 *    There is no relationship between the thread that executes the ``lf::join``
 *    and the thread that resumes the coroutine.
 *
 * \endrst
 */
inline constexpr impl::join_type join = {};

} // namespace core

namespace impl {

/**
 * @brief An awaitable (in a `lf::task`) that triggers a rethrow of the internal exception (if any).
 *
 * This is designed for use in combination with `lf::call` when you are not inside a fork-join scope.
 */
inline constexpr impl::rethrow_if_exception_type rethrow_if_exception = {};

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asynchronous function.
 */
template <tag Tag, modifier_for<Tag> Mod>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <quasi_pointer I, async_function_object F>
  LF_DEPRECATE_CALL [[nodiscard]] LF_STATIC_CALL auto operator()(I ret, F fun) LF_STATIC_CONST {
    return combinate<Tag, Mod>(std::move(ret), std::move(fun));
  }

  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <async_function_object F>
  LF_DEPRECATE_CALL [[nodiscard]] LF_STATIC_CALL auto operator()(F fun) LF_STATIC_CONST {
    return combinate<Tag, Mod>(discard_t{}, std::move(fun));
  }

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <quasi_pointer I, async_function_object F>
  [[nodiscard]] LF_STATIC_CALL auto operator[](I ret, F fun) LF_STATIC_CONST {
    return combinate<Tag, Mod>(std::move(ret), std::move(fun));
  }

  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <async_function_object F>
  [[nodiscard]] LF_STATIC_CALL auto operator[](F fun) LF_STATIC_CONST {
    return combinate<Tag, Mod>(discard_t{}, std::move(fun));
  }
#endif
};

} // namespace impl

inline namespace core {

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 *
 * Conceptually the forked/child task can be executed anywhere at anytime and
 * and in parallel with its continuation.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no guaranteed relationship between the thread that executes the ``lf::fork``
 *    and the thread(s) that execute the continuation/child. However, currently ``libfork``
 *    uses continuation stealing so the thread that calls ``lf::fork`` will immediately begin
 *    executing the child.
 *
 * \endrst
 */
inline constexpr impl::bind_task<tag::fork, impl::modifier::none> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 *
 * Conceptually the called/child task can be executed anywhere at anytime but, its
 * continuation is guaranteed to be sequenced after the child returns.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no relationship between the thread that executes the ``lf::call`` and
 *    the thread(s) that execute the continuation/child. However, currently ``libfork``
 *    uses continuation stealing so the thread that calls ``lf::call`` will immediately
 *    begin executing the child.
 *
 * \endrst
 */
inline constexpr impl::bind_task<tag::call, impl::modifier::none> call = {};

} // namespace core

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
