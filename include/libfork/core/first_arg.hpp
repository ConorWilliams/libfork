#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <type_traits>

#include "libfork/core/promise_base.hpp"
#include "libfork/macro.hpp"

/**
 * @file first_arg.hpp
 *
 * @brief Implementation of the type that is the first argument to all coroutines.
 */

namespace lf {

/**
 * @brief The first argument to all async functions will be passes a type derived from this class.
 *
 * If ``AsyncFn`` is an ``async_fn`` then this will derive from ``async_fn``. If ``AsyncFn`` is an ``async_mem_fn``
 * then this will wrap a pointer to a class and will supply the appropriate ``*`` and ``->`` operators.
 *
 * The full type of the first argument will also have a static ``context()`` member function that will defer to the
 * thread context's ``context()`` member function.
 */
template <tag Tag, typename AsyncFn, typename... Self>
  requires(sizeof...(Self) <= 1)
struct first_arg;

namespace detail {

/**
 * @brief An awaitable type (in a task) that triggers a join.
 */
struct join_t {};

/**
 * @brief An awaitable type (in a task) that triggers a fork/call.
 */
template <typename R, typename Head, typename... Tail>
struct [[nodiscard]] packet {
private:
  struct empty {};

public:
  [[no_unique_address]] std::conditional_t<std::is_void_v<R>, empty, std::add_lvalue_reference_t<R>> ret;
  [[no_unique_address]] Head context;
  [[no_unique_address]] std::tuple<Tail &&...> args;
};

template <typename T>
concept not_first_arg = !requires { typename std::decay_t<T>::lf_is_first_arg; };

} // namespace detail

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct async_fn {
  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <typename... Args>
  LIBFORK_STATIC_CALL constexpr auto operator()(Args &&...args) LIBFORK_STATIC_CONST noexcept -> detail::packet<void, first_arg<tag::invoke, async_fn<Fn>>, Args...> {
    return {{}, {}, {std::forward<Args>(args)...}};
  }
};

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct async_mem_fn {
  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <detail::not_first_arg Self, typename... Args>
  LIBFORK_STATIC_CALL constexpr auto operator()(Self &self, Args &&...args) LIBFORK_STATIC_CONST noexcept -> detail::packet<void, first_arg<tag::invoke, async_mem_fn<Fn>, Self>, Args...> {
    return {{}, {self}, {std::forward<Args>(args)...}};
  }
};

// -------------------------------------------------------------------------- //

/**
 * @brief A specialization of ``first_arg`` for asynchronous global functions.
 */
template <tag Tag, stateless F>
struct first_arg<Tag, async_fn<F>> : async_fn<F> {
  /**
   * @brief The type of the underlying asynchronous function originally wrapped by ``async[_mem]_fn``.
   */
  using underlying_async_fn = F;
  /**
   * @brief The value of the ``Tag`` template parameter.
   */
  static constexpr tag tag_value = Tag;
};

/**
 * @brief A specialization of ``first_arg`` for asynchronous member functions.
 */
template <tag Tag, stateless F, typename This>
  requires(!std::is_reference_v<This>)
struct first_arg<Tag, async_mem_fn<F>, This> {
  /**
   * @brief The type of the underlying asynchronous function originally wrapped by ``async[_mem]_fn``.
   */
  using underlying_async_fn = F;
  /**
   * @brief The value of the ``Tag`` template parameter.
   */
  static constexpr tag tag_value = Tag;
  /**
   * @brief A tag to detect this type.
   */
  using lf_is_first_arg = void;
  /**
   * @brief Construct a ``first_arg`` from a reference to ``this``.
   */
  explicit(false) constexpr first_arg(This &self) : m_self{std::addressof(self)} {} // NOLINT
  /**
   * @brief Access the underlying ``this`` pointer.
   */
  [[nodiscard]] constexpr auto operator->() noexcept -> This * { return m_self; }
  /**
   * @brief Deference the underlying ``this`` pointer.
   */
  [[nodiscard]] constexpr auto operator*() noexcept -> This & { return *m_self; }

private:
  This *m_self;
};

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */
