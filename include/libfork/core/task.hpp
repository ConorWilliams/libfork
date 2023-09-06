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
#include <utility>

#include "libfork/macro.hpp"

#include "libfork/core/core.hpp"

/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

// ----------------------------------------------- //

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 */
enum class tag {
  root, ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call, ///< Non root task (on a virtual stack) from an ``lf::call``.
  fork, ///< Non root task (on a virtual stack) from an ``lf::fork``.
};

// ---------------------- Concepts ------------------------- //

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of tasks.
 * The stack of ``lf::async_stack``s is expected to never be empty, it should always
 * be able to return an empty ``lf::async_stack``.
 */
template <typename Context>
concept thread_context = requires(Context ctx, owner<async_stack *> stack, non_null<task_block *> handle) {
  { ctx.max_threads() } -> std::same_as<std::size_t>;

  { ctx.stack_pop() } -> std::convertible_to<owner<async_stack *>>;
  { ctx.stack_push(stack) };

  { ctx.task_pop() } -> std::convertible_to<task_block *>;
  { ctx.task_push(handle) };
};

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

/**
 * @brief Disable rvalue references for T&& template types if an async function is forked.
 *
 * This is to prevent the user from accidentally passing a temporary object to an async function that
 * will then destructed in the parent task before the child task returns.
 */
template <typename T, typename Self>
concept no_forked_rvalue = Self::tag_value != tag::fork || std::is_reference_v<T>;

// ---------------------------------------------------------- //

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct [[nodiscard]] async_fn {
  /**
   * @brief Use with explicit template-parameter.
   */
  consteval async_fn() = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide generated from this.
   */
  explicit(false) consteval async_fn([[maybe_unused]] Fn invocable_which_returns_a_task) {} // NOLINT
};

// ------------------------ Forward decl ------------------------ //

namespace detail {

template <typename R, typename T, thread_context Context, tag Tag>
struct promise_type;

} // namespace detail

template <typename T = void>
class task;

// ------------------------ Interfaces ------------------------ //

namespace detail {

/**
 * @brief A type that satisfies the ``thread_context`` concept.
 *
 * This is used to detect bad coroutine calls early. All its methods are
 * unimplemented as it is only used in unevaluated contexts.
 */
struct dummy_context {
  auto max_threads() -> std::size_t;

  auto stack_pop() -> owner<async_stack *>;
  auto stack_push(owner<async_stack *>) -> void;

  auto task_pop() -> task_block *;
  auto task_push(non_null<task_block *>) -> void;
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

/**
 * @brief A specialization of ``first_arg_t`` for asynchronous functions.
 *
 * This derives from the global function to allow to allow for use as a y-combinator.
 */
template <typename R, tag Tag, stateless F>
struct first_arg_t : async_fn<F>, move_only<first_arg_t<R, Tag, F>> {
  using context_type = dummy_context;
  using return_address_t = R;
  using underlying_fn = F;
  static constexpr tag tag_value = Tag;
};

namespace detail {
// Detect if a type is a specialization of ``first_arg_t``.

template <typename T>
struct is_first_arg : std::false_type {};

template <typename R, tag Tag, stateless F>
struct is_first_arg<first_arg_t<R, Tag, F>> : std::true_type {};

} // namespace detail

template <typename T>
concept first_arg = std::is_empty_v<std::remove_cvref_t<T>> && detail::is_first_arg<std::remove_cvref_t<T>>::value;

template <typename T>
concept not_first_arg = !first_arg<T>;

// ------------------------ Packet ------------------------ //

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

// clang-format off

template <typename R, tag Tag, typename TaskValueType>
concept result_matches = std::is_void_v<R> || Tag == tag::root || std::assignable_from<R &, TaskValueType>;

template <typename Head, typename... Tail>
concept valid_packet = first_arg<Head> && requires(typename Head::underlying_fn fun, Head head, Tail &&...tail) {
  //  
  { std::invoke(fun, std::move(head), std::forward<Tail>(tail)...) } -> is_task;

} && result_matches<typename Head::return_address_t, Head::tag_value, typename std::invoke_result_t<typename Head::underlying_fn, Head, Tail...>::value_type>;

// clang-format on

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 *
 * This will store a patched version of Head that includes the return type.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
struct [[nodiscard]] packet {
  //
  using return_type = typename Head::return_address_t;
  using task_type = typename std::invoke_result_t<typename Head::underlying_fn, Head, Tail...>;
  using value_type = typename task_type::value_type;
  using promise_type = typename stdx::coroutine_traits<task_type, Head, Tail &&...>::promise_type;
  using handle_type = typename stdx::coroutine_handle<promise_type>;

  [[no_unique_address]] std::conditional_t<std::is_void_v<return_type>, detail::empty, std::add_lvalue_reference_t<return_type>> ret;
  [[no_unique_address]] Head context;
  [[no_unique_address]] std::tuple<Tail &&...> args;

  [[no_unique_address]] immovable _anon = {}; // NOLINT

  /**
   * @brief Call the underlying async function and return a handle to it, sets the return address if ``R != void``.
   */
  auto invoke_bind(stdx::coroutine_handle<promise_base> parent) && -> handle_type {

    LF_ASSERT(parent || Head::tag_value == tag::root);

    auto unwrap = [&]<class... Args>(Args &&...xargs) -> handle_type {
      return handle_type::from_address(std::invoke(typename Head::underlying_fn{}, std::move(context), std::forward<Args>(xargs)...).m_handle);
    };

    handle_type child = std::apply(unwrap, std::move(args));

    child.promise().set_parent(parent);

    if constexpr (!std::is_void_v<return_type>) {
      child.promise().set_ret_address(ret);
    }

    return child;
  }
};

} // namespace detail

// ----------------------------- Task ----------------------------- //

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T>
class task {
public:
  using value_type = T; ///< The type of the value returned by the coroutine (cannot be a reference, use ``std::reference_wrapper``).

private:
  template <typename Head, typename... Tail>
    requires detail::valid_packet<Head, Tail...>
  friend struct detail::packet;

  template <typename, typename, thread_context, tag>
  friend struct detail::promise_type;

  // Promise constructs, packets accesses.
  explicit constexpr task(non_null<frame_block *> handle) noexcept : m_handle{handle} {
    LF_ASSERT(handle != nullptr);
  }

  non_null<frame_block *> m_handle; ///< The handle to the coroutine.
};

// ----------------------------- first_arg_t impl ----------------------------- //

namespace detail {

} // namespace detail

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */
