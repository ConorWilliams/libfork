#ifndef DD0B4328_55BD_452B_A4A5_5A4670A6217B
#define DD0B4328_55BD_452B_A4A5_5A4670A6217B

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for invocable, constructible_from, convertible_to
#include <functional>  // for invoke
#include <type_traits> // for invoke_result_t, remove_cvref_t
#include <utility>     // for forward

#include "libfork/core/ext/context.hpp"  // for worker_context, full_context
#include "libfork/core/ext/tls.hpp"      // for context
#include "libfork/core/impl/frame.hpp"   // for frame
#include "libfork/core/impl/utility.hpp" // for different_from, referenceable
#include "libfork/core/macro.hpp"        // for LF_COMPILER_EXCEPTIONS, LF_FORCEINLINE
#include "libfork/core/tag.hpp"          // for tag

/**
 * @file first_arg.hpp
 *
 * @brief Machinery for the (library-generated) first argument of async functions.
 */

namespace lf {

inline namespace core {

/**
 * @brief Test if the expression `*std::declval<T&>()` is valid and has a referenceable type i.e. non-void.
 */
template <typename I>
concept dereferenceable = requires (I val) {
  { *val } -> impl::referenceable;
};

/**
 * @brief A quasi-pointer if a movable type that can be dereferenced to a referenceable type type i.e.
 * non-void.
 *
 * A quasi-pointer is assumed to be cheap-to-move like an iterator/legacy-pointer.
 */
template <typename I>
concept quasi_pointer = std::default_initializable<I> && std::movable<I> && dereferenceable<I>;

/**
 * @brief A concept that requires a type be a copyable [function
 * object](https://en.cppreference.com/w/cpp/named_req/FunctionObject).
 *
 * An async function object is a function object that returns an `lf::task` when `operator()` is called.
 * with appropriate arguments. The call to `operator()` must create a libfork coroutine. The first argument
 * of an async function must accept a deduced templated-type that satisfies the `lf::core::first_arg` concept.
 * The return type and invocability of an async function must be independent of the first argument except
 * for its tag value.
 *
 * An async function may be copied, its copies must be equivalent to the original and support concurrent
 * invocation from multiple threads. It is assumed that an async function is cheap-to-copy like
 * an iterator/legacy-pointer.
 */
template <typename F>
concept async_function_object =                         //
    std::is_class_v<std::remove_cvref_t<F>> &&          // Only classes can have templated operator().
    std::copy_constructible<std::remove_cvref_t<F>> &&  // Must be able to copy/move a value.
    std::constructible_from<std::remove_cvref_t<F>, F>; // Must be able to convert to a value.

/**
 * @brief This describes the public-API of the first argument passed to an async function.
 *
 * An async functions' invocability and return type must be independent of their first argument except for
 * its tag value. A user may query the first argument's static member `tagged` to obtain this value.
 * Additionally, a user may query the first argument's static member function `context()` to obtain a
 * pointer to the current workers context. Finally a user may cache an exception in-flight by calling
 * `.stash_exception()`.
 */
template <typename T>
concept first_arg = std::is_class_v<T> && async_function_object<T> && requires (T arg) {
  { T::tagged } -> std::convertible_to<tag>;
  { T::context() } -> std::same_as<worker_context *>;
  { arg.stash_exception() } noexcept;
};

} // namespace core

namespace impl {

/**
 * @brief The type passed as the first argument to async functions.
 *
 * Its functions are:
 *
 * - Act as a y-combinator (expose same invocability as F).
 * - Provide a handle to the coroutine frame for exception handling.
 * - Statically inform the return pointer type.
 * - Statically provide the tag.
 * - Statically provide the calling argument types.
 *
 * Hence, a first argument is also an async function object.
 */
template <quasi_pointer I, tag Tag, async_function_object F, typename... CallArgs>
  requires std::is_class_v<F> && (std::is_reference_v<CallArgs> && ...)
class first_arg_t {
 public:
  /**
   * @brief Tag indicating how the async function was called.
   */
  static constexpr tag tagged = Tag;

  /**
   * @brief Get the current workers context.
   */
  [[nodiscard]] static auto context() -> worker_context * { return tls::context(); }

  /**
   * @brief Stash an exception that will be rethrown at the end of the next join.
   */
  void stash_exception() const noexcept {
#if LF_COMPILER_EXCEPTIONS
    m_frame->capture_exception();
#endif
  }

  /**
   * @brief Construct a first_arg_t from an async function object.
   */
  template <different_from<first_arg_t> T>
    requires std::constructible_from<F, T>
  explicit first_arg_t(T &&expr) noexcept(std::is_nothrow_constructible_v<F, T>)
      : m_fun(std::forward<T>(expr)) {}

  /**
   * @brief Forward call to the underlying async function object.
   */
  template <typename... Args>
    requires std::invocable<F &, Args...>
  auto operator()(Args &&...args) & noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F &, Args...> {
    return std::invoke(m_fun, std::forward<Args>(args)...);
  }

  /**
   * @brief Forward call to the underlying async function object.
   */
  template <typename... Args>
    requires std::invocable<F const &, Args...>
  auto operator()(Args &&...args) const & noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F const &, Args...> {
    return std::invoke(m_fun, std::forward<Args>(args)...);
  }

  /**
   * @brief Forward call to the underlying async function object.
   */
  template <typename... Args>
    requires std::invocable<F &&, Args...>
  auto operator()(Args &&...args) && noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F &&, Args...> {
    return std::invoke(std::move(m_fun), std::forward<Args>(args)...);
  }

  /**
   * @brief Forward call to the underlying async function object.
   */
  template <typename... Args>
    requires std::invocable<F const &&, Args...>
  auto operator()(Args &&...args) const && noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F const &&, Args...> {
    return std::invoke(std::move(m_fun), std::forward<Args>(args)...);
  }

 private:
  /**
   * @brief Hidden friend reduces discoverability, this is an implementation detail.
   */
  [[nodiscard]] friend auto unwrap(first_arg_t &&arg) noexcept -> F && { return std::move(arg.m_fun); }

  /**
   * @brief Hidden friend reduces discoverability, this is an implementation detail.
   */
  LF_FORCEINLINE friend auto unsafe_set_frame(first_arg_t &arg, frame *frame) noexcept {
#if LF_COMPILER_EXCEPTIONS
    arg.m_frame = frame;
#endif
  }

  [[no_unique_address]] F m_fun;
#if LF_COMPILER_EXCEPTIONS
  frame *m_frame;
#endif
};

} // namespace impl

} // namespace lf

#endif /* DD0B4328_55BD_452B_A4A5_5A4670A6217B */
