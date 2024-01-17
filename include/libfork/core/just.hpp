#ifndef DE1C62F1_949F_48DC_BC2C_960C4439332D
#define DE1C62F1_949F_48DC_BC2C_960C4439332D

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception> // for rethrow_exception
#include <utility>   // for forward

#include "libfork/core/eventually.hpp"      // for try_eventually
#include "libfork/core/first_arg.hpp"       // for async_function_object
#include "libfork/core/impl/awaitables.hpp" // for call_awaitable
#include "libfork/core/impl/combinate.hpp"  // for combinate
#include "libfork/core/impl/frame.hpp"      // for frame
#include "libfork/core/invocable.hpp"       // for invocable, invoke_result_t
#include "libfork/core/macro.hpp"           // for LF_STATIC_CALL, LF_STATI...
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
  try_eventually<R> ret;
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
 * @brief An invocable (and subscriptable) wrapper that makes an async function object immediately callable.
 */
struct bind_just {
  /**
   * @brief Make an async function object immediate callable.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a call + join.
   */
  template <async_function_object F>
  LF_DEPRECATE_CALL [[nodiscard]] LF_STATIC_CALL auto operator()(F fun) LF_STATIC_CONST {
    return [fun = std::move(fun)]<typename... Args>(Args &&...args) mutable
      requires invocable<F, Args...>
    {
      return impl::just_awaitable<invoke_result_t<F, Args...>>(std::move(fun), std::forward<Args>(args)...);
    };
  }

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a call + join.
   */
  template <async_function_object F>
  [[nodiscard]] LF_STATIC_CALL auto operator[](F fun) LF_STATIC_CONST {
    return [fun = std::move(fun)]<typename... Args>(Args &&...args) mutable
      requires invocable<F, Args...>
    {
      return impl::just_awaitable<invoke_result_t<F, Args...>>(std::move(fun), std::forward<Args>(args)...);
    };
  }
#endif
};

} // namespace impl

inline namespace core {

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call +
 * join.
 */
inline constexpr impl::bind_just just = {};

} // namespace core

} // namespace lf

#endif /* DE1C62F1_949F_48DC_BC2C_960C4439332D */
