#ifndef DE1C62F1_949F_48DC_BC2C_960C4439332D
#define DE1C62F1_949F_48DC_BC2C_960C4439332D

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for invocable
#include <coroutine>   // for suspend_never
#include <exception>   // for rethrow_exception
#include <functional>  // for invoke
#include <type_traits> // for invoke_result_t
#include <utility>     // for forward

#include "libfork/core/control_flow.hpp"
#include "libfork/core/eventually.hpp"      // for try_eventually
#include "libfork/core/first_arg.hpp"       // for async_function_object
#include "libfork/core/impl/awaitables.hpp" // for call_awaitable
#include "libfork/core/impl/combinate.hpp"  // for combinate
#include "libfork/core/impl/frame.hpp"      // for frame
#include "libfork/core/invocable.hpp"       // for async_invocable, async_result_t
#include "libfork/core/macro.hpp"           // for LF_STATIC_CALL, LF_STATIC_CONST, LF_DEPRECAT...
#include "libfork/core/tag.hpp"             // for tag
#include "libfork/core/task.hpp"            // for returnable

/**
 * @file just.hpp
 *
 * @brief Implementation of immediate invocation wrapper.
 */

namespace lf {

namespace impl {

/**
 * @brief A base class that provides a ``ret`` member.
 */
template <returnable R>
struct just_awaitable_base {
  /**
   * @brief The return variable.
   */
  [[no_unique_address]] try_eventually<R> ret;
};

/**
 * @brief An awaitable that triggers a call + join.
 */
template <returnable R>
class [[nodiscard("co_await this!")]] just_awaitable : just_awaitable_base<R>, call_awaitable {

  // clang-format off

 public:
 /**
  * @brief Construct a new just join awaitable binding the return address to an internal member.
  */
  template <typename Fun, typename... Args>
  explicit just_awaitable(Fun &&fun, Args &&...args)
      : call_awaitable{
            {}, combinate<tag::call>(&this->ret, std::forward<Fun>(fun))(std::forward<Args>(args)...).prom
        } 
      {}

  // clang-format on

  /**
   * @brief Access the frame of the child task.
   */
  auto frame() const noexcept -> frame * { return this->child; }

  using call_awaitable::await_ready;

  using call_awaitable::await_suspend;

  /**
   * @brief Return the result of the asynchronous function or rethrow the exception.
   */
  auto await_resume() -> R {

    if (this->ret.has_exception()) {
      std::rethrow_exception(std::move(this->ret).exception());
    }

    if constexpr (!std::is_void_v<R>) {
      return *std::move(this->ret);
    }
  }
};

/**
 * @brief A wrapper around a returned value that will be passed through an `co_await`.
 */
template <typename T>
struct [[nodiscard("co_await this!")]] just_wrapped : std::suspend_never {
  /**
   * @brief Forward the result.
   */
  constexpr auto await_resume() noexcept -> T && {
    if constexpr (std::is_lvalue_reference_v<T>) {
      return val;
    } else {
      return std::move(val);
    }
  }

  [[no_unique_address]] T val; ///< The value to be forwarded.
};

/**
 * @brief Void specialization of ``just_wrapped``.
 */
template <>
struct just_wrapped<void> : std::suspend_never {};

/**
 * @brief A wrapper that supplies an async function with a call operator.
 */
template <typename F>
  requires (!std::is_reference_v<F>)
struct [[nodiscard("This should be immediately invoked!")]] call_just {

  /**
   * @brief Make an awaitable that will call the async function then immediately join.
   */
  template <typename... Args>
    requires async_invocable<F, Args...>
  auto operator()(Args &&...args) && -> just_awaitable<async_result_t<F, Args...>> {
    return just_awaitable<async_result_t<F, Args...>>(std::move(fun), std::forward<Args>(args)...);
  }

  /**
   * @brief Immediately invoke a regular function and wrap the result in an awaitable class.
   */
  template <typename... Args>
    requires std::invocable<F, Args...> && (!async_invocable<F, Args...>)
  auto operator()(Args &&...args) && -> just_wrapped<std::invoke_result_t<F, Args...>> {
    if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
      std::invoke(std::move(fun), std::forward<Args>(args)...);
      return {};
    } else {
      return {{}, std::invoke(std::move(fun), std::forward<Args>(args)...)};
    }
  }

  [[no_unique_address]] F fun; ///< The async or regular function.
};

/**
 * @brief An invocable (and subscriptable) wrapper that makes an async function object immediately callable.
 */
struct bind_just {
  /**
   * @brief Make an async function object immediate callable.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a call + join.
   */
  template <typename F>
  LF_DEPRECATE_CALL LF_STATIC_CALL auto operator()(F fun) LF_STATIC_CONST->call_just<F> {
    return {std::move(fun)};
  }

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a call + join.
   */
  template <typename F>
  LF_STATIC_CALL auto operator[](F fun) LF_STATIC_CONST->call_just<F> {
    return {std::move(fun)};
  }
#endif
};

} // namespace impl

inline namespace core {

/**
 * @brief A second-order functor, produces an awaitable (in an ``lf::task``) that will trigger a call + join.
 */
inline constexpr impl::bind_just just = {};

} // namespace core

} // namespace lf

#endif /* DE1C62F1_949F_48DC_BC2C_960C4439332D */
