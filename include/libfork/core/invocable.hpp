#ifndef A5349E86_5BAA_48EF_94E9_F0EBF630DE04
#define A5349E86_5BAA_48EF_94E9_F0EBF630DE04

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for invocable, same_as
#include <iterator>    // for indirectly_writable
#include <type_traits> // for true_type, type_identity, inv...

#include "libfork/core/eventually.hpp" // for eventually
#include "libfork/core/exception.hpp"
#include "libfork/core/first_arg.hpp" // for first_arg_t, quasi_pointer
#include "libfork/core/impl/utility.hpp"
#include "libfork/core/tag.hpp"  // for tag
#include "libfork/core/task.hpp" // for task, returnable

/**
 * @file invocable.hpp
 *
 * @brief A collection concepts that extend ``std::invocable`` to async functions.
 */

namespace lf {

namespace impl {

/**
 * @brief A type which can be assigned any value as a noop.
 *
 * Useful to ignore a value tagged with ``[[no_discard]]``.
 */
struct ignore_t {
  /**
   * @brief A no-op assignment operator.
   */
  constexpr void operator=([[maybe_unused]] auto const &discard) const noexcept {}
};

/**
 * @brief A tag type to indicate an async function's return value will be discarded by the caller.
 *
 * This type is indirectly writable from any value.
 */
struct discard_t {
  /**
   * @brief Return a proxy object that can be assigned any value.
   */
  constexpr auto operator*() -> ignore_t { return {}; }
};

// ------------ Bare-bones inconsistent invocable ------------ //

namespace detail {

// Base case: invalid
template <typename I, typename Task>
struct valid_return : std::false_type {};

// Special case: discard_t is valid for void
template <>
struct valid_return<discard_t, task<void>> : std::true_type {};

// Special case: stash_exception_in_return + void
template <stash_exception_in_return I>
struct valid_return<I, task<void>> : std::true_type {};

// Anything indirectly_writable
template <returnable R, std::indirectly_writable<R> I>
struct valid_return<I, task<R>> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that `Task` is an lf::task and that the result can be returned by `I`.
 *
 * This requires that `I` is `std::indirectly_writable` or that `I` is `discard_t` and the task returns void.
 */
template <typename I, typename Task>
inline constexpr bool valid_return_v = quasi_pointer<I> && detail::valid_return<I, Task>::value;

/**
 * @brief Verify that `R` returned via `I`.
 *
 * This requires that `I` is `std::indirectly_writable` or that `I` is `discard_t` and the `R` is void.
 */
template <typename I, typename R>
concept return_address_for = quasi_pointer<I> && returnable<R> && valid_return_v<I, task<R>>;

/**
 * @brief Verify `F` is async `Tag` invocable with `Args...` and returns a task who's result type is
 * returnable via I.
 */
template <typename I, tag Tag, typename F, typename... Args>
concept async_invocable_to_task =
    quasi_pointer<I> &&                                                                                    //
    async_function_object<F> &&                                                                            //
    std::invocable<F, impl::first_arg_t<I, Tag, F, Args &&...>, Args...> &&                                //
    valid_return_v<I, std::invoke_result_t<F, impl::first_arg_t<discard_t, Tag, F, Args &&...>, Args...>>; //

/**
 * @brief Let `F(Args...) -> task<R>` then this returns 'R'.
 *
 * Unsafe in the sense that it does not check that F is `async_invocable`.
 */
template <typename I, tag Tag, typename F, typename... Args>
  requires async_invocable_to_task<I, Tag, F, Args...>
struct unsafe_result {
  using type = std::invoke_result_t<F, impl::first_arg_t<I, Tag, F, Args...>, Args...>::type;
};

/**
 * @brief Let `F(Args...) -> task<R>` then this returns 'R'.
 *
 * Unsafe in the sense that it does not check that F is `async_invocable`.
 */
template <typename I, tag Tag, typename F, typename... Args>
  requires async_invocable_to_task<I, Tag, F, Args...>
using unsafe_result_t = typename unsafe_result<I, Tag, F, Args...>::type;

// --------------------- //

/**
 * @brief Check that F can be 'Tag'-invoked to produce `task<R>`.
 */
template <typename R, typename I, tag T, typename F, typename... Args>
concept return_exactly =                                //
    async_invocable_to_task<I, T, F, Args...> &&        //
    std::same_as<R, unsafe_result_t<I, T, F, Args...>>; //

/**
 * @brief Check that `F` can be `T`-invoked and called with `Args...`.
 */
template <typename R, typename I, tag T, typename F, typename... Args>
concept call_consistent =                        //
    return_exactly<R, I, T, F, Args...> &&       //
    return_exactly<R, I, tag::call, F, Args...>; //

namespace detail {

template <typename R, bool Exception>
struct as_eventually : std::type_identity<basic_eventually<R, Exception> *> {};

template <>
struct as_eventually<void, false> : std::type_identity<discard_t> {};

} // namespace detail

/**
 * @brief Wrap R in an basic_eventually if it is not void.
 */
template <typename R, bool Exception>
using as_eventually_t = typename detail::as_eventually<R, Exception>::type;

/**
 * @brief Check that `Tag`-invoking and calling `F` with `Args...` produces task<R>.
 *
 * This also checks the results is consistent when it is discarded and returned
 * by `basic_eventually<...> *`.
 */
template <typename R, typename I, tag T, typename F, typename... Args>
concept self_consistent =                                          //
    call_consistent<R, I, T, F, Args...> &&                        //
    call_consistent<R, discard_t, T, F, Args...> &&                //
    call_consistent<R, as_eventually_t<R, true>, T, F, Args...> && //
    call_consistent<R, as_eventually_t<R, false>, T, F, Args...>;  //

// --------------------- //

/**
 * @brief Check `F` is async invocable to a task with `I`,` discard_t` and the appropriate `eventually`s.
 */
template <typename I, tag Tag, typename F, typename... Args>
concept consistent_invocable =                                                //
    async_invocable_to_task<I, Tag, F, Args...> &&                            //
    self_consistent<unsafe_result_t<I, Tag, F, Args...>, I, Tag, F, Args...>; //

// --------------------- //

} // namespace impl

inline namespace core {

/**
 * @brief Check `F` is `Tag`-invocable with `Args...` and returns an `lf::task` who's result is returnable via
 * `I`.
 *
 * In the following description "invoking" or "async invoking" means to call `F` with `Args...` via the
 * appropriate libfork function i.e. `fork` corresponds to `lf::fork[r, f](args...)` and the library will
 * generate the appropriate (opaque) first-argument.
 *
 * This requires:
 *  - `F` is 'Tag'/call invocable with `Args...` when writing the result to `I` or discarding it.
 *  - The result of all of these calls has the same type.
 *  - The result of all of these calls is an instance of type `lf::task<R>`.
 *  - `I` is movable and dereferenceable.
 *  - `I` is indirectly writable from `R` or `R` is `void` while `I` is `discard_t`.
 *  - If `R` is non-void then `F` is `lf::core::async_invocable` when `I` is `lf::basic_eventually<R, ?> *`.
 *
 * This concept is provided as a building block for higher-level concepts.
 */
template <typename I, tag Tag, typename F, typename... Args>
concept async_invocable = impl::consistent_invocable<I, Tag, F, Args...>;

// --------- //

/**
 * @brief Alias for `lf::core::async_invocable<lf::impl::discard_t, lf::core::tag::call, F, Args...>`.
 */
template <typename F, typename... Args>
concept invocable = async_invocable<impl::discard_t, tag::call, F, Args...>;

/**
 * @brief Alias for `lf::core::async_invocable<lf::impl::discard_t, lf::core::tag::root, F, Args...>`,
 * subsumes `lf::core::invocable`.
 */
template <typename F, typename... Args>
concept rootable = invocable<F, Args...> && async_invocable<impl::discard_t, tag::root, F, Args...>;

/**
 * @brief Alias for `lf::core::async_invocable<lf::impl::discard_t, lf::core::tag::fork, F, Args...>`,
 * subsumes `lf::core::invocable`.
 */
template <typename F, typename... Args>
concept forkable = invocable<F, Args...> && async_invocable<impl::discard_t, tag::fork, F, Args...>;

// --------- //

/**
 * @brief Fetch `R` when the async function `F` returns `lf::task<R>`.
 */
template <typename F, typename... Args>
  requires invocable<F, Args...>
struct invoke_result : impl::unsafe_result<impl::discard_t, tag::call, F, Args...> {};

/**
 * @brief Fetch `R` when the async function `F` returns `lf::task<R>`.
 */
template <typename F, typename... Args>
  requires invocable<F, Args...>
using invoke_result_t = typename invoke_result<F, Args...>::type;

} // namespace core

} // namespace lf

#endif /* A5349E86_5BAA_48EF_94E9_F0EBF630DE04 */
