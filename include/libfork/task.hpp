#ifndef C9921D3E_28E4_4577_BB9C_E7CA55766E92
#define C9921D3E_28E4_4577_BB9C_E7CA55766E92

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include "libfork/coroutine.hpp"
#include "libfork/macro.hpp"
#include "libfork/promise.hpp"
#include "libfork/promise_base.hpp"

/**
 * @file task.hpp
 *
 * @brief The task class and associated utilities.
 */

namespace lf {

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T, thread_context Context>
  requires(!std::is_reference_v<T>)
class task : public stdexp::coroutine_handle<detail::promise_base<T>> {
public:
  using value_type = T;         ///< The type of the value returned by the coroutine.
  using context_type = Context; ///< The type of the context in which the coroutine is executed.

private:
  explicit constexpr task(auto handle) noexcept : stdexp::coroutine_handle<detail::promise_base<T>>{handle} {}

  template <typename, thread_context, detail::tag>
  friend struct detail::promise_type;
};

namespace detail {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T, thread_context Context>
struct is_task_impl<task<T, Context>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

template <typename F, typename... Args>
using invoke_t = std::invoke_result_t<F, detail::magic<detail::root_t, wrap_fn<F>>, Args...>;

} // namespace detail

/**
 * @brief The entry point for syncronous execution of a coroutine.
 *
 * This will create the coroutine and pass its handle to ``schedule``. ``schedule`` is expected to garantee that
 * some thread will call ``resume()`` on the coroutine handle it is passed. The calling thread will then block
 * until the task has finished executing. Finally the result of the task will be returned.
 */
template <std::invocable<stdexp::coroutine_handle<>> Schedule, stateless F, class... Args, detail::is_task Task = detail::invoke_t<F, Args...>>
  requires std::default_initializable<typename Task::value_type> || std::is_void_v<typename Task::value_type>
auto sync_wait(Schedule &&schedule, wrap_fn<F>, Args &&...args) -> typename Task::value_type {

  using value_type = typename Task::value_type;

  task const coro = std::invoke(F{}, detail::magic<detail::root_t, wrap_fn<F>>{}, std::forward<Args>(args)...);

  detail::root_block_t root_block;

  coro.promise().control_block.set(root_block);

  [[maybe_unused]] std::conditional_t<std::is_void_v<value_type>, int, value_type> result;

  if constexpr (!std::is_void_v<value_type>) {
    coro.promise().set_return_address(result);
  }

#if LIBFORK_COMPILER_EXCEPTIONS
  try {
    std::invoke(std::forward<Schedule>(schedule), stdexp::coroutine_handle<>{coro});
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
  root_block.acquire();

  root_block.rethrow_if_unhandled();

  if constexpr (!std::is_void_v<value_type>) {
    return result;
  }
}

} // namespace lf

#endif /* C9921D3E_28E4_4577_BB9C_E7CA55766E92 */
