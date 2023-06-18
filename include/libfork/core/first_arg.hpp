#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>

#include "libfork/core/promise_base.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"
#include "libfork/macro.hpp"

/**
 * @file first_arg.hpp
 *
 * @brief Implementation of the type that is the first argument to all coroutines.
 */

namespace lf {

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

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
 * @brief A type that satisfies the ``thread_context`` concept.
 */
struct dummy_context {
  using stack_type = virtual_stack<64>;

  static auto context() -> dummy_context &;

  auto max_threads() -> std::size_t;

  auto stack_top() -> typename stack_type::handle;
  auto stack_pop() -> void;
  auto stack_push(typename stack_type::handle) -> void;

  auto task_pop() -> std::optional<task_handle>;
  auto task_push(task_handle) -> void;
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

template <typename Arg>
concept is_first_arg = requires {
                         // Explicit opt-in.
                         typename Arg::lf_is_first_arg;

                         // Functional requirements.
                         typename Arg::context_type;
                         typename Arg::underlying_async_fn;
                         { Arg::tag_value } -> std::same_as<tag const &>;

                         requires stateless<typename Arg::underlying_async_fn>;
                         requires thread_context<typename Arg::context_type>;
                       };

template <stateless F, tag Tag>
struct first_arg_base {
  using lf_is_first_arg = std::true_type;
  using context_type = detail::dummy_context;
  using underlying_async_fn = F;
  static constexpr tag tag_value = Tag;
};

template <typename T>
concept not_first_arg = !
is_first_arg<std::remove_cvref_t<T>>;

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

template <is_first_arg Head, typename... Tail>
using invoke_t = std::invoke_result_t<typename Head::underlying_async_fn, Head, Tail...>;

template <is_first_arg Head, typename... Tail>
using task_value_t = typename invoke_t<Head, Tail...>::value_type;

template <typename R, is_first_arg Head>
inline constexpr bool is_deferred = (Head::tag_value == tag::invoke || Head::tag_value == tag::root) && std::is_void_v<R>;

/**
 * @brief An awaitable type (in a task) that triggers a fork/call.
 */
template <typename R, is_first_arg Head, typename... Tail>
  requires is_task<invoke_t<Head, Tail...>> && (std::same_as<R, task_value_t<Head, Tail...>> || is_deferred<R, Head>)
struct [[nodiscard]] packet {
private:
  struct empty {};

public:
  using task_type = typename std::invoke_result_t<typename Head::underlying_async_fn, Head, Tail...>;
  using value_type = typename task_type::value_type;
  using promise_type = typename stdexp::coroutine_traits<task_type, Head, Tail &&...>::promise_type;
  using handle_type = typename stdexp::coroutine_handle<promise_type>;

  [[no_unique_address]] std::conditional_t<std::is_void_v<R>, empty, std::add_lvalue_reference_t<value_type>> ret;
  [[no_unique_address]] Head context;
  [[no_unique_address]] std::tuple<Tail &&...> args;

  /**
   * @brief Call the underlying async function and return a handle to it, set the return address if ``R != void``.
   */
  auto invoke_bind() && -> handle_type {

    auto unwrap = [&]<class... Args>(Args &&...xargs) -> handle_type {
      return handle_type::from_address(std::invoke(typename Head::underlying_async_fn{}, context, std::forward<Args>(xargs)...).handle);
    };

    handle_type child = std::apply(unwrap, std::move(args));

    if constexpr (!std::is_void_v<R>) {
      child.promise().set_ret_address(std::addressof(ret));
    }

    return child;
  }
};

} // namespace detail

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct async_fn {
  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   *
   * An invoke should not be triggered inside a ``fork``/``call``/``join`` region as the exceptions
   * will be muddled, use ``lf:call`` instead.
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
   *
   * An invoke should not be triggered inside a ``fork``/``call``/``join`` region as the exceptions
   * will be muddled, use ``lf::call`` instead.
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
struct first_arg<Tag, async_fn<F>> : detail::first_arg_base<F, Tag>, async_fn<F> {};

/**
 * @brief A specialization of ``first_arg`` for asynchronous member functions.
 */
template <tag Tag, stateless F, typename This>
  requires(!std::is_reference_v<This>)
struct first_arg<Tag, async_mem_fn<F>, This> : detail::first_arg_base<F, Tag> {
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
