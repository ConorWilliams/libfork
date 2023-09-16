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

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"

/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

inline namespace core {

/**
 * @brief A fixed string type for template parameters that tracks its source location.
 */
template <typename Char, std::size_t N>
struct tracked_fixed_string {
 private:
  using sloc = std::source_location;
  static constexpr std::size_t file_name_max_size = 127;

 public:
  consteval tracked_fixed_string(Char const (&str)[N], sloc loc = sloc::current()) noexcept
      : line{loc.line()},
        column{loc.column()} {
    for (std::size_t i = 0; i < N; ++i) {
      function_name[i] = str[i];
    }

    // std::size_t count = 0 loc.while
  }

  std::array<Char, N> function_name;
  // std::array<char, file_name_max_size + 1> file_name_buf;
  // std::size_t file_name_size;
  std::uint_least32_t line;
  std::uint_least32_t column;
};

} // namespace core

// ----------------------------------------------- //

inline namespace core {

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * IMPORTANT: The value type ``T`` of a coroutine should not be a function of its first argument.
 */
template <typename T = void, tracked_fixed_string Name = "">
struct task {
  using value_type = T; ///< The type of the value returned by the coroutine.

  task(frame_block *frame) : m_frame{non_null(frame)} {}

  [[nodiscard]] constexpr auto frame() const noexcept -> frame_block * { return m_frame; }

 private:
  frame_block *m_frame; ///< The frame block for the coroutine.
};

} // namespace core

namespace impl {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T, auto Name>
struct is_task_impl<task<T, Name>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

} // namespace impl

inline namespace core {

// ----------------------------------------------- //

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 *
 * You can inspect the first arg of an async function to determine the tag.
 */
enum class tag {
  root,   ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call,   ///< Non root task (on a virtual stack) from an ``lf::call``, completes synchronously.
  fork,   ///< Non root task (on a virtual stack) from an ``lf::fork``, completes asynchronously.
  invoke, ///< Equivalent to ``call`` but caches the return (extra move required).
  tail,   ///< Force a tail-call optimization.
};

// ----------------------------------------------- //

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::context_type`.
 */
template <typename T>
  requires requires { requires thread_context<typename std::remove_cvref_t<T>::context_type>; }
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

// ----------------------------------------------- //

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

// ----------------------------------------------- //

/**
 * @brief Forward decl for concepts.
 */
template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async;

/**
 * @brief The API of the first arg passed to an async function.
 */
template <typename Arg>
concept first_arg = impl::unqualified<Arg> && std::is_trivially_copyable_v<Arg> && requires(Arg arg) {
  //
  tag_of<Arg>;

  typename context_of<Arg>;
  typename return_of<Arg>;
  typename function_of<Arg>;

  requires !std::is_reference_v<return_of<Arg>>;

  { std::remove_cvref_t<Arg>::context() } -> std::same_as<context_of<Arg> *>;

  requires impl::is_void<return_of<Arg>> || requires {
    { arg.address() } -> std::convertible_to<return_of<Arg> *>;
  };

  []<typename F>(async<F>)
    requires std::same_as<F, function_of<Arg>>
  {
    // Check implicitly convertible to async and that deduced template parameter is the correct type.
  }
  (arg);
};

} // namespace core

namespace impl {

// ----------------------------------------------- //

template <typename T>
concept not_first_arg = !first_arg<T>;

// ----------------------------------------------- //

template <typename Task, typename Head>
concept valid_return = is_task<Task> && valid_result<return_of<Head>, value_of<Task>>;

/**
 * @brief Check that the async function encoded in `Head` is invocable with arguments in `Tail`.
 */
template <typename Head, typename... Tail>
concept valid_packet = first_arg<Head> && valid_return<std::invoke_result_t<function_of<Head>, Head, Tail...>, Head>;

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

  [[nodiscard]] static auto context() -> Context * { return non_null(tls::ctx<Context>); }
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
  constexpr packet(Head head, Tail &&...tail) noexcept : m_args{std::move(head), std::forward<Tail>(tail)...} {}

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke(frame_block *parent) && -> frame_block *requires(tag_of<Head> != tag::root) {
    auto tsk = std::apply(function_of<Head>{}, std::move(m_args));
    tsk.frame()->set_parent(parent);
    return tsk.frame();
  }

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke() && -> frame_block *requires(tag_of<Head> == tag::root) {
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

} // namespace impl

// ----------------------------------------------- //

inline namespace core {

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async {
  /**
   * @brief Use with explicit template-parameter.
   */
  consteval async() = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide
   * generated from this.
   *
   * This is to allow concise definitions from lambdas:
   *
   * .. code::
   *
   *    constexpr async fib = [](auto fib, ...) -> task<int, "fib"> {
   *        // ...
   *    };
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
   *
   * Note that the return type is tagged void however during the `await_transform` the full type deduced.
   */
  template <typename... Args>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept -> invoke_packet<Args...> {
    return {{}, std::forward<Args>(args)...};
  }

  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <typename... Args>
    requires impl::is_void<value_of<invoke_packet<Args...>>>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept -> call_packet<Args...> {
    return {{}, std::forward<Args>(args)...};
  }
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
  auto max_threads() -> std::size_t;
  auto submit(frame_block *) -> void;
  auto task_pop() -> frame_block *;
  auto task_push(frame_block *) -> void;
  auto stack_pop() -> async_stack *;
  auto stack_push(async_stack *) -> void;
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

  [[nodiscard]] static auto context() -> context_type * { LF_THROW(std::runtime_error{"Should never be called!"}); }
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg : basic_first_arg<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  constexpr basic_first_arg(return_type &ret) : m_ret{std::addressof(ret)} {}

  [[nodiscard]] constexpr auto address() const noexcept -> return_type * { return m_ret; }

 private:
  R *m_ret;
};

static_assert(first_arg<basic_first_arg<void, tag::root, decltype([] {})>>);
static_assert(first_arg<basic_first_arg<int, tag::root, decltype([] {})>>);

} // namespace impl

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */
