#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/coroutine.hpp"
#include "libfork/promise.hpp"
#include "libfork/promise_base.hpp"
#include "libfork/task.hpp"
#include <type_traits>

/**
 * @file libfork.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` and ``lf::sync_wait`` machinery.
 */

#ifndef LIBFORK_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <typename T, typename Context, auto Tag, typename... F, typename... Args>
struct lf::stdexp::coroutine_traits<lf::task<T, Context>, lf::detail::magic<Tag, F...>, Args...> {
  using promise_type = lf::detail::promise_type<T, Context, Tag>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <typename T, typename Context, typename This, auto Tag, typename... F, typename... Args>
struct lf::stdexp::coroutine_traits<lf::task<T, Context>, This, lf::detail::magic<Tag, F...>, Args...> {
  using promise_type = lf::detail::promise_type<T, Context, Tag>;
};

#endif /* LIBFORK_DOXYGEN_SHOULD_SKIP_THIS */

namespace lf {

namespace detail {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T, thread_context Context>
struct is_task_impl<task<T, Context>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

template <typename F, typename... Args>
using invoke_t = std::invoke_result_t<F, detail::magic<tag::root, async_fn<F>>, Args...>;

} // namespace detail

// // clang-format off

namespace detail {

// template <tag Tag, typename F, typename... This, typename... Args>
// auto invoke(magic<Tag, F, This...> head, Args &&...tail) {

//   using magic_t = magic<Tag, F, This...>;

//   using task_type = std::invoke_result_t<F, magic_t, Args &&...>;

//   using promise_type = stdexp::coroutine_traits<task_type, magic_t, Args &&...>::promise_type;

//   using handle_type = stdexp::coroutine_handle<promise_type>;

//   task_type const coro = std::invoke(F{}, head, std::forward<Args>(tail)...);

//   return handle_type::from_address(coro.m_handle);
// }

}; // namespace detail

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 *
 * This will create the coroutine and pass its handle to ``schedule``. ``schedule`` is expected to guarantee that
 * some thread will call ``resume()`` on the coroutine handle it is passed. The calling thread will then block
 * until the asynchronous function has finished executing. Finally the result of the asynchronous function
 * will be returned.
 */
template <std::invocable<stdexp::coroutine_handle<>> Schedule, async_wrapper F, class... Args>
auto sync_wait(Schedule &&schedule, [[maybe_unused]] F async, Args &&...args) {

  // clang-format on

  using magic_t = detail::magic<detail::tag::root, F>;

  using task_type = std::invoke_result_t<typename F::type, magic_t, Args &&...>;

  using value_type = typename task_type::value_type;

  using promise_type = stdexp::coroutine_traits<task_type, magic_t, Args &&...>::promise_type;

  using handle_type = stdexp::coroutine_handle<promise_type>;

  task_type const coro = std::invoke(typename F::type{}, magic_t{}, std::forward<Args>(args)...);

  handle_type const handle = handle_type::from_address(coro.m_handle);

  detail::root_block_t<value_type> root_block;

  // Set address of root block.
  handle.promise().set(static_cast<void *>(&root_block));

#if LIBFORK_COMPILER_EXCEPTIONS
  try {
    std::invoke(std::forward<Schedule>(schedule), stdexp::coroutine_handle<>{handle});
  } catch (...) {
    // We cannot know whether the coroutine has been resumed or not once we pass to schedule(...).
    // Hence, we do not know whether or not to .destroy() it if schedule(...) throws.
    // Hence we mark noexcept to trigger termination.
    []() noexcept {
      throw;
    }();
  }
#else
  std::invoke(std::forward<Schedule>(schedule), stdexp::coroutine_handle<>{coro});
#endif

  // Block until the coroutine has finished.
  root_block.m_semaphore.acquire();

  root_block.rethrow_if_unhandled();

  if constexpr (!std::is_void_v<value_type>) {
    return std::move(*root_block.m_result);
  }
}

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asynchronous function.
 */
template <template <typename...> typename Packet>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] LIBFORK_STATIC_CALL constexpr auto operator()(R &ret, async_fn<F>) LIBFORK_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> Packet<R, F, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }

  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] LIBFORK_STATIC_CALL constexpr auto operator()(async_fn<F>) LIBFORK_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> Packet<void, F, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }

#if defined(LIBFORK_DOXYGEN_SHOULD_SKIP_THIS) || defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] static constexpr auto operator[](R &ret, async_fn<F> func) noexcept {
    return bind_task{}(ret, func);
  }

  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] static constexpr auto operator[](async_fn<F> func) noexcept {
    return bind_task{}(func);
  }
#endif
};

/**
 * @brief An awaitable (in a task) that triggers a join.
 */
inline constexpr detail::join_t join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 */
inline constexpr bind_task<detail::fork_packet> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr bind_task<detail::call_packet> call = {};

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
