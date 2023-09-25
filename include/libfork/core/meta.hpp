#ifndef D66BBECE_E467_4EB6_B74A_AAA2E7256E02
#define D66BBECE_E467_4EB6_B74A_AAA2E7256E02

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>

#include "libfork/core/list.hpp"
#include "libfork/core/macro.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/utility.hpp"

/**
 * @file meta.hpp
 *
 * @brief Provides interfaces and meta programming utilities.
 */
namespace lf {

inline namespace ext {

// ----------------------------------------------------- //

/**
 * @brief An alias for a `submit_h<Context> *` stored in a linked list.
 */
template <typename Context>
using intruded_h = intrusive_node<submit_h<Context> *>;

// ------------------------------------------------------ //

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of
 * tasks. The stack of ``lf::async_stack``s is expected to never be empty, it
 * should always be able to return an empty ``lf::async_stack``.
 *
 * Syntactically a `thread_context` requires:
 *
 * \rst
 *
 * .. include:: ../../include/libfork/core/meta.hpp
 *    :code:
 *    :start-line: 56
 *    :end-line: 64
 *
 * \endrst
 */
template <typename Context>
concept thread_context = requires (Context ctx, async_stack *stack, intruded_h<Context> *ext, task_h<Context> *task) {
  { ctx.max_threads() } -> std::same_as<std::size_t>;           // The maximum number of threads.
  { ctx.submit(ext) };                                          // Submit an external task to the context.
  { ctx.task_pop() } -> std::convertible_to<task_h<Context> *>; // If the stack is empty, return a null pointer.
  { ctx.task_push(task) };                                      // Push a non-null pointer.
  { ctx.stack_pop() } -> std::convertible_to<async_stack *>;    // Return a non-null pointer
  { ctx.stack_push(stack) };                                    // Push a non-null pointer
};

namespace detail {

// clang-format off

template <thread_context Context>
static consteval auto always_single_threaded() -> bool {
  if constexpr (requires { Context::max_threads(); }) {
    if constexpr (impl::constexpr_callable<[] { Context::max_threads(); }>) {
      return Context::max_threads() == 1;
    }
  }
  return false;
}

// clang-format on

} // namespace detail

template <typename Context>
concept single_thread_context = thread_context<Context> && detail::always_single_threaded<Context>();

} // namespace ext

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
  invoke, ///< Equivalent to ``lf::call`` but caches the return (extra move required).
};

// ------------------------ Helpers ----------------------- //

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

// -------------------------- Forward declaration -------------------------- //

/**
 * @brief Check if an invocable is suitable for use as an `lf::async` function.
 *
 * This requires `T` to be:
 *
 * - A class type.
 * - Trivially copiable.
 * - Default initializable.
 * - Empty.
 */
template <typename T>
concept stateless =
    std::is_class_v<T> && std::is_trivially_copyable_v<T> && std::is_empty_v<T> && std::default_initializable<T>;

// See "async.hpp" for the definition.

template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async;

// -------------------------- First arg static interface -------------------------- //

} // namespace core

namespace impl {

/**
 * @brief Detect what kind of async function a type can be cast to.
 */
template <stateless T>
consteval auto implicit_cast_to_async(async<T>) -> T {
  return {};
}

} // namespace impl

inline namespace core {

/**
 * @brief The API of the first argument passed to an async function.
 *
 * All async functions must have a templated first arguments, this argument will be generated by the compiler and encodes
 * many useful/queryable properties. A full specification is give below:
 *
 * \rst
 *
 * .. include:: ../../include/libfork/core/meta.hpp
 *    :code:
 *    :start-line: 198
 *    :end-line: 218
 *
 * \endrst
 */
template <typename Arg>
concept first_arg = impl::unqualified<Arg> && requires (Arg arg) {
  //
  requires std::is_trivially_copyable_v<Arg>;

  tag_of<Arg>;

  typename context_of<Arg>;
  typename return_of<Arg>;
  typename function_of<Arg>;

  requires !std::is_reference_v<return_of<Arg>>;

  { std::remove_cvref_t<Arg>::context() } -> std::same_as<context_of<Arg> *>;

  requires impl::is_void<return_of<Arg>> || requires {
    { arg.address() } -> std::convertible_to<return_of<Arg> *>;
  };

  { impl::implicit_cast_to_async(arg) } -> std::same_as<function_of<Arg>>;
};

} // namespace core

namespace impl {

/**
 * @brief The negation of `first_arg`.
 */
template <typename T>
concept not_first_arg = !first_arg<T>;

/**
 * @brief Check if a type is a `first_arg` with a specific tag.
 */
template <typename Arg, tag Tag>
concept first_arg_tagged = first_arg<Arg> && tag_of<Arg> == Tag;

} // namespace impl

} // namespace lf

#endif /* D66BBECE_E467_4EB6_B74A_AAA2E7256E02 */
