#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "libfork/core/macro.hpp"
#include "libfork/core/meta.hpp"
#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/utility.hpp"

/**
 * @file async.hpp
 *
 * @brief Implementation of the core ``lf::task`` and `lf::async` types.
 */

namespace lf {

inline namespace core {

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other coroutines.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should ever touch an instance of this type, it is used for specifying the
 *    return type of an `async` function only.
 *
 * .. warning::
 *    The value type ``T`` of a coroutine should never be a function of its context or return address type.
 *
 * \endrst
 */
template <typename T = void>
struct task {

  using value_type = T; ///< The type of the value returned by the coroutine.

  /**
   * @brief __Not__ part of the public API.
   *
   * This should only be called by the compiler.
   */
  constexpr task(impl::frame_block *frame) noexcept : m_frame{non_null(frame)} {}

  /**
   * @brief __Not__ part of the public API.
   */
  [[nodiscard]] constexpr auto frame() const noexcept -> impl::frame_block * { return m_frame; }

 private:
  impl::frame_block *m_frame; ///< The frame block for the coroutine.
};

} // namespace core

namespace impl {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

// ----------------------------------------------- //

template <typename Task, typename Head>
concept valid_return = is_task<Task> && valid_result<return_of<Head>, value_of<Task>>;

/**
 * @brief Check that the async function encoded in `Head` is invocable with arguments in `Tail`.
 */
template <typename Head, typename... Tail>
concept valid_packet =
    first_arg<Head> && valid_return<std::invoke_result_t<function_of<Head>, Head, Tail...>, Head>;

/**
 * @brief A base class for building the first argument to asynchronous functions.
 *
 * This derives from `async<F>` to allow to allow for use as a y-combinator.
 *
 * It needs the true context type to be patched to it.
 *
 * This is used by `stdx::coroutine_traits` to build the promise type.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg;

/**
 * @brief A helper to statically attach a new `context_type` to a `first_arg`.
 */
template <thread_context Context, first_arg Head>
struct patched : Head {

  using context_type = Context;

  /**
   * @brief Get a pointer to the thread-local context.
   *
   * \rst
   *
   * .. warning::
   *    This may change at every ``co_await``!
   *
   * \endrst
   */
  [[nodiscard]] static auto context() noexcept -> Context * { return non_null(tls::get_ctx<Context>()); }
};

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
class [[nodiscard("packets must be co_awaited")]] packet : move_only<packet<Head, Tail...>> {
 public:
  using task_type = std::invoke_result_t<function_of<Head>, Head, Tail...>;
  using value_type = value_of<task_type>;

  /**
   * @brief Build a packet.
   *
   * It is implicitly constructible because we specify the return type for SFINE and we don't want to
   * repeat the type.
   *
   */
  constexpr packet(Head head, Tail &&...tail) noexcept
      : m_args{std::move(head), std::forward<Tail>(tail)...} {}

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke(frame_block *parent) && -> frame_block *requires (tag_of<Head> != tag::root) {
    task_type tsk = std::apply(function_of<Head>{}, std::move(m_args));
    tsk.frame()->set_parent(parent);
    return tsk.frame();
  }

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke() && -> frame_block *requires (tag_of<Head> == tag::root) {
    return std::apply(function_of<Head>{}, std::move(m_args)).frame();
  }

  template <typename F>
  constexpr auto apply(F &&func) && -> decltype(auto) {
    return std::apply(std::forward<F>(func), std::move(m_args));
  }

  /**
   * @brief Patch the `Head` type with `Context`
   */
  template <thread_context Context>
  constexpr auto patch_with() && noexcept -> packet<patched<Context, Head>, Tail...> {
    return std::move(*this).apply([](Head head, Tail &&...tail) -> packet<patched<Context, Head>, Tail...> {
      return {{std::move(head)}, std::forward<Tail>(tail)...};
    });
  }

 private:
  [[no_unique_address]] std::tuple<Head, Tail &&...> m_args;
};

namespace detail {

template <typename Packet>
struct repack {};

template <typename Head, typename... Args>
using swap_head = basic_first_arg<eventually<value_of<packet<Head, Args...>>>, tag::call, function_of<Head>>;

template <first_arg_tagged<tag::invoke> Head, typename... Args>
  requires valid_packet<swap_head<Head, Args...>, Args...>
struct repack<packet<Head, Args...>> : std::type_identity<packet<swap_head<Head, Args...>, Args...>> {
  static_assert(std::is_void_v<return_of<Head>>, "Only void packets are expected to be repacked");
};

} // namespace detail

template <typename Packet>
using repack_t = typename detail::repack<Packet>::type;

/**
 * @brief Check if a void invoke packet with `value_type` `X` can be converted to a call packet with
 * `return_type` `eventually<X>` without changing the `value_type` of the new packet.
 */
template <typename Packet>
concept repackable = non_void<value_of<Packet>> && requires { typename detail::repack<Packet>::type; } &&
                     std::same_as<value_of<Packet>, value_of<repack_t<Packet>>>;

} // namespace impl

// ----------------------------------------------- //

inline namespace core {

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 *
 * Use this alongside `lf::task` to define an synchronous function.
 */
template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async {
  /**
   * @brief For use with an explicit template-parameter.
   */
  consteval async() = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide
   * generated from this.
   *
   * \rst
   *
   * This is to allow concise definitions from lambdas:
   *
   * .. code::
   *
   *    constexpr async fib = [](auto fib, ...) -> task<int> {
   *        // ...
   *    };
   *
   * \endrst
   */
  consteval async([[maybe_unused]] Fn invocable_which_returns_a_task) {}

 private:
  template <typename... Args>
  using invoke_packet = impl::packet<impl::basic_first_arg<void, tag::invoke, Fn>, Args...>;

  template <typename... Args>
  using call_packet = impl::packet<impl::basic_first_arg<void, tag::call, Fn>, Args...>;

 public:
  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <typename... Args>
    requires impl::repackable<invoke_packet<Args...>>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept
      -> invoke_packet<Args...> {
    return {{}, std::forward<Args>(args)...};
  }

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <typename... Args>
    requires impl::is_void<value_of<invoke_packet<Args...>>>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept -> call_packet<Args...> {
    return {{}, std::forward<Args>(args)...};
  }

#endif
};

} // namespace core

// ----------------------------------------------- //

namespace impl {

/**
 * @brief A type that satisfies the ``thread_context`` concept.
 *
 * This is used to detect bad coroutine calls early. All its methods are
 * unimplemented as it is only used in unevaluated contexts.
 */
struct dummy_context {
  auto max_threads() -> std::size_t;                ///< Unimplemented.
  auto submit(intruded_h<dummy_context> *) -> void; ///< Unimplemented.
  auto task_pop() -> task_h<dummy_context> *;       ///< Unimplemented.
  auto task_push(task_h<dummy_context> *) -> void;  ///< Unimplemented.
  auto stack_pop() -> async_stack *;                ///< Unimplemented.
  auto stack_push(async_stack *) -> void;           ///< Unimplemented.
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

/**
 * @brief Void/ignore specialization.
 */
template <tag Tag, stateless F>
struct basic_first_arg<void, Tag, F> : async<F>, private move_only<basic_first_arg<void, Tag, F>> {
  using context_type = dummy_context;   ///< A default context
  using return_type = void;             ///< The type of the return address.
  using function_type = F;              ///< The underlying async
  static constexpr tag tag_value = Tag; ///< The tag value.

  /**
   * @brief Unimplemented - to satisfy the ``thread_context`` concept.
   */
  [[nodiscard]] static auto context() noexcept -> context_type *;
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg : basic_first_arg<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  constexpr basic_first_arg(return_type &ret) noexcept : m_ret{std::addressof(ret)} {}

  [[nodiscard]] constexpr auto address() const noexcept -> return_type * { return m_ret; }

 private:
  R *m_ret;
};

static_assert(first_arg<basic_first_arg<void, tag::root, decltype([] {})>>);
static_assert(first_arg<basic_first_arg<int, tag::root, decltype([] {})>>);

} // namespace impl

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */
