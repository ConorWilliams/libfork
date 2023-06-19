#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>
#include <utility>

#include "libfork/core/coroutine.hpp"
#include "libfork/core/first_arg.hpp"
#include "libfork/core/promise.hpp"
#include "libfork/core/promise_base.hpp"
#include "libfork/core/task.hpp"

/**
 * @file libfork.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` and ``lf::sync_wait`` machinery.
 */

// clang-format off

#ifndef LIBFORK_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <typename T, typename Head, typename... Args>
  requires std::same_as<Head, std::decay_t<Head>> && requires {
    typename Head::context_type;
    Head::tag_value;
  }
struct lf::stdx::coroutine_traits<lf::task<T>, Head, Args...> {
  using promise_type = lf::detail::promise_type<T, typename Head::context_type, Head::tag_value>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <typename T, typename Self, typename Head, typename... Args>
  requires std::same_as<Head, std::decay_t<Head>> && requires {
    typename Head::context_type;
    Head::tag_value;
  }
struct lf::stdx::coroutine_traits<lf::task<T>, Self, Head, Args...> {
  using promise_type = lf::detail::promise_type<T, typename Head::context_type, Head::tag_value>;
};

#endif /* LIBFORK_DOXYGEN_SHOULD_SKIP_THIS */

namespace lf {

/**
 * @brief A concept which requires a type to define a ``context_type`` which satisfy ``lf::thread_context``.
 */
template <typename T>
concept defines_context = requires { typename std::decay_t<T>::context_type; } && thread_context<typename std::decay_t<T>::context_type>;

/**
 * @brief A concept which defines the requirements for a scheduler.
 * 
 * This requires a type to define a ``context_type`` which satisfies ``lf::thread_context`` and have a ``schedule`` method
 * which accepts a ``std::coroutine_handle<>`` and guarantees some-thread will call it's ``resume()`` member.
 */
template <typename Scheduler>
concept scheduler = defines_context<Scheduler> && requires(Scheduler &&scheduler) {
  std::forward<Scheduler>(scheduler).schedule(stdx::coroutine_handle<>{});
};

// clang-format on

/**
 * @brief Builds an async function from a stateless invocable that returns an ``lf::task``.
 *
 * Use this to define a global function which is passed a copy of itself as its first parameter (e.g. a y-combinator).
 */
template <stateless Fn>
[[nodiscard]] consteval auto fn([[maybe_unused]] Fn invocable_which_returns_a_task) -> async_fn<Fn> { return {}; }

/**
 * @brief Builds an async member function from a stateless invocable that returns an ``lf::task``.
 *
 * Use this to define a member function which is passed a pointer to an instance of the class as its first parameter.
 */
template <stateless Fn>
[[nodiscard]] consteval auto mem_fn([[maybe_unused]] Fn invocable_which_returns_a_task) -> async_mem_fn<Fn> { return {}; }

// NOLINTEND

namespace detail {

template <scheduler Schedule, typename Head, class... Args>
auto sync_wait_impl(Schedule &&scheduler, Head head, Args &&...args) {

  using packet_t = packet<void, Head, Args...>;

  root_block_t<typename packet_t::value_type> root_block;

  auto handle = packet_t{{}, head, {std::forward<Args>(args)...}}.invoke_bind();

  LIBFORK_LOG("Set root address {}", (std::size_t)&root_block);

  // Set address of root block.
  handle.promise().set_ret_address(&root_block);

#if LIBFORK_COMPILER_EXCEPTIONS
  try {
    std::forward<Schedule>(scheduler).schedule(stdx::coroutine_handle<>{handle});
  } catch (...) {
    // We cannot know whether the coroutine has been resumed or not once we pass to schedule(...).
    // Hence, we do not know whether or not to .destroy() it if schedule(...) throws.
    // Hence we mark noexcept to trigger termination.
    []() noexcept {
      throw;
    }();
  }
#else
  std::forward<Schedule>(scheduler).schedule(stdx::coroutine_handle<>{handle});
#endif

  // Block until the coroutine has finished.
  root_block.semaphore.acquire();

  root_block.exception.rethrow_if_unhandled();

  if constexpr (!std::is_void_v<typename packet_t::value_type>) {
    LIBFORK_ASSERT(root_block.result.has_value());
    return std::move(*root_block.result);
  }
}

template <scheduler S, typename AsyncFn, typename... Self>
struct as_root : with_context<typename std::decay_t<S>::context_type, first_arg<tag::root, AsyncFn, Self...>> {};

} // namespace detail

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 *
 * This will create the coroutine and pass its handle to ``scheduler``'s  ``schedule`` method. The caller
 * will then block until the asynchronous function has finished executing.
 * Finally the result of the asynchronous function will be returned to the caller.
 */
template <scheduler S, stateless F, class... Args>
[[nodiscard]] auto sync_wait(S &&scheduler, [[maybe_unused]] async_fn<F> async_function, Args &&...args) {
  return detail::sync_wait_impl(std::forward<S>(scheduler), detail::as_root<S, async_fn<F>>{}, std::forward<Args>(args)...);
}

/**
 * @brief The entry point for synchronous execution of asynchronous member functions.
 *
 * This will create the coroutine and pass its handle to ``scheduler``'s  ``schedule`` method. The caller
 * will then block until the asynchronous member function has finished executing.
 * Finally the result of the asynchronous member function will be returned to the caller.
 */
template <scheduler S, stateless F, class Self, class... Args>
[[nodiscard]] auto sync_wait(S &&scheduler, [[maybe_unused]] async_mem_fn<F> async_member_function, Self &self, Args &&...args) {
  return detail::sync_wait_impl(std::forward<S>(scheduler), detail::as_root<S, async_mem_fn<F>, Self>{self}, std::forward<Args>(args)...);
}

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asynchronous function.
 */
template <tag Tag>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] LIBFORK_STATIC_CALL constexpr auto
  operator()(R &ret, [[maybe_unused]] async_fn<F> async) LIBFORK_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<R, first_arg<Tag, async_fn<F>>, Args...> {
      return {{ret}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] LIBFORK_STATIC_CALL constexpr auto operator()([[maybe_unused]] async_fn<F> async) LIBFORK_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<void, first_arg<Tag, async_fn<F>>, Args...> {
      return {{}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Bind return address `ret` to an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] LIBFORK_STATIC_CALL constexpr auto operator()(R &ret, [[maybe_unused]] async_mem_fn<F> async) LIBFORK_STATIC_CONST noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self &self, Args &&...args) noexcept -> detail::packet<R, first_arg<Tag, async_mem_fn<F>, Self>, Args...> {
      return {{ret}, {self}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] LIBFORK_STATIC_CALL constexpr auto operator()([[maybe_unused]] async_mem_fn<F> async) LIBFORK_STATIC_CONST noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self &self, Args &&...args) noexcept -> detail::packet<void, first_arg<Tag, async_mem_fn<F>, Self>, Args...> {
      return {{}, {self}, {std::forward<Args>(args)...}};
    };
  }

#if defined(LIBFORK_DOXYGEN_SHOULD_SKIP_THIS) || defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] static constexpr auto operator[](R &ret, [[maybe_unused]] async_fn<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<R, first_arg<Tag, async_fn<F>>, Args...> {
      return {{ret}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] static constexpr auto operator[]([[maybe_unused]] async_fn<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<void, first_arg<Tag, async_fn<F>>, Args...> {
      return {{}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Bind return address `ret` to an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] static constexpr auto operator[](R &ret, [[maybe_unused]] async_mem_fn<F> async) noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self &self, Args &&...args) noexcept -> detail::packet<R, first_arg<Tag, async_mem_fn<F>, Self>, Args...> {
      return {{ret}, {self}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] static constexpr auto operator[]([[maybe_unused]] async_mem_fn<F> async) noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self &self, Args &&...args) noexcept -> detail::packet<void, first_arg<Tag, async_mem_fn<F>, Self>, Args...> {
      return {{}, {self}, {std::forward<Args>(args)...}};
    };
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
inline constexpr bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr bind_task<tag::call> call = {};

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
