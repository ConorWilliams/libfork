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

#include "libfork/core/stack.hpp"

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
  root,   ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call,   ///< Non root task (on a virtual stack) from an ``lf::call``, completes synchronously.
  fork,   ///< Non root task (on a virtual stack) from an ``lf::fork``, completes asynchronously.
  invoke, ///< Equivalent to ``call`` but caches the return (extra move required).
};

// ---------------------- Concepts ------------------------- //

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of
 * tasks. The stack of ``lf::async_stack``s is expected to never be empty, it
 * should always be able to return an empty ``lf::async_stack``.
 */
template <typename Context>
concept thread_context =
    requires(Context ctx, owner<detail::async_stack *> stack, non_null<task_ptr> handle) {
      { ctx.max_threads() } -> std::same_as<std::size_t>;

      { ctx.stack_pop() } -> std::convertible_to<owner<detail::async_stack *>>;
      { ctx.stack_push(stack) };

      { ctx.task_pop() } -> std::convertible_to<task_ptr>;
      { ctx.task_push(handle) };
    };

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

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

  consteval async_fn(async_fn const &) = default;
  consteval async_fn(async_fn &&) noexcept = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide
   * generated from this.
   *
   * This is to allow concise definitions from lambdas:
   *
   * .. code::
   *
   *    constexpr async_fn fib = [](auto fib, ...){
   *        // ...
   *    };
   */
  explicit(false) consteval async_fn([[maybe_unused]] Fn invocable_which_returns_a_task) {}

  auto operator=(async_fn const &) -> async_fn & = delete;
  auto operator=(async_fn &&) -> async_fn & = delete;

  ~async_fn() = default;
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
  auto task_pop() -> task_ptr;
  auto task_push(non_null<task_ptr>) -> void;
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

/**
 * @brief A base class for building the first argument to asynchronous functions.
 *
 * This derives from `async_fn<F>` to allow to allow for use as a y-combinator.
 *
 * This is used by `std::coroutine_traits` to build the promise type.
 */
template <typename R, tag Tag, stateless F>
struct first_arg_base;

/**
 * @brief Void specialization.
 */
template <tag Tag, stateless F>
struct first_arg_base<void, Tag, F> : async_fn<F>, move_only<first_arg_base<void, Tag, F>> {
  using context_type = dummy_context;   ///< A default context
  using return_type = void;             ///< The type of the return address.
  using function_type = F;              ///< The underlying async_fn
  static constexpr tag tag_value = Tag; ///< The tag value.
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct first_arg_base : first_arg_base<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  explicit constexpr first_arg_base(return_type &ret) : m_ret{std::addressof(ret)} {}

  constexpr auto address() const noexcept -> return_type * { return m_ret; }

private:
  R *m_ret;
};

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::context_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::context_type; }
using context_of = typename std::remove_cvref_t<T>::context_type;

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::return_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::return_type; }
using return_of = typename std::remove_cvref_t<T>::return_type;

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::function_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::function_type; }
using function_of = typename std::remove_cvref_t<T>::function_type;

/**
 * @brief A helper to fetch `std::remove_cvref_t<T>::tag_value`.
 */
template <typename T>
  requires requires {
    { std::remove_cvref_t<T>::tag_value } -> std::convertible_to<tag>;
  }
inline constexpr tag tag_of = std::remove_cvref_t<T>::tag_value;

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::value_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::value_type; }
using value_of = typename std::remove_cvref_t<T>::value_type;

/**
 * @brief The API of the first arg passed to an async function.
 */
template <typename Arg>
concept first_arg = requires(Arg arg) {
  //
  tag_of<Arg>;

  typename context_of<Arg>;
  typename return_of<Arg>;
  typename function_of<Arg>;

  requires std::is_void_v<return_of<Arg>> || requires {
    { arg.address() } -> std::convertible_to<return_of<Arg> *>;
  };

  []<typename F>(async_fn<F>)
    requires std::same_as<F, function_of<Arg>>
  {
    // Check implicitly convertible to async_fn and that deduced template parameter is the correct type.
  }
  (arg);
};

static_assert(first_arg<first_arg_base<void, tag::root, decltype([] {})>>);
static_assert(first_arg<first_arg_base<int, tag::root, decltype([] {})>>);

template <typename T>
concept not_first_arg = !first_arg<T>;

/**
 * @brief Disable rvalue references for T&& template types if an async function
 * is forked.
 *
 * This is to prevent the user from accidentally passing a temporary object to
 * an async function that will then destructed in the parent task before the
 * child task returns.
 */
template <typename T, typename Self>
concept protect_forwarding_tparam = first_arg<Self> && !std::is_rvalue_reference_v<T> &&
                                    (tag_of<Self> != tag::fork || std::is_reference_v<T>);

// ------------------------ Packet ------------------------ //

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

template <typename R, tag Tag, typename TaskValueType>
concept result_matches = std::is_void_v<R> || Tag == tag::root || std::assignable_from<R &, TaskValueType>;

template <typename Head, typename... Tail>
concept valid_packet =
    first_arg<Head> && is_task<std::invoke_result_t<function_of<Head>, Head, Tail...>> &&
    result_matches<return_of<Head>, tag_of<Head>,
                   value_of<std::invoke_result_t<typename Head::underlying_fn, Head, Tail...>>>;

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 *
 * This will store a patched version of Head that includes the return type.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
struct [[nodiscard]] packet : move_only<packet<Head, Tail...>> {
private:
  using return_t = return_of<Head>;
  using return_p = std::conditional_t<std::is_void_v<return_t>, empty, std::add_lvalue_reference_t<return_t>>;

  using task_t = typename std::invoke_result_t<return_t, Head, Tail &&...>;
  using promise_type = typename stdx::coroutine_traits<task_t, Head, Tail &&...>::promise_type;
  using handle_type = typename stdx::coroutine_handle<promise_type>;

public:
  [[no_unique_address]] return_p ret;
  [[no_unique_address]] Head context;
  [[no_unique_address]] std::tuple<Tail &&...> args;

  /**
   * @brief Call the underlying async function and return a handle to it., sets the return address if ``R !=
   * void``.
   */
  auto invoke_bind() && -> handle_type {
    LF_ASSERT(parent || Head::tag_value == tag::root);

    auto unwrap = [&]<class... Args>(Args &&...xargs) -> handle_type {
      return handle_type::from_address(
          std::invoke(typename Head::underlying_fn{}, std::move(context), std::forward<Args>(xargs)...)
              .m_handle);
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
  using value_type = T; ///< The type of the value returned by the coroutine.

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

// ----------------------------- first_arg_t impl -----------------------------
// //

namespace detail {} // namespace detail

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */
