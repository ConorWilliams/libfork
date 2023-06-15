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

#include "libfork/core/coroutine.hpp"
#include "libfork/core/promise_base.hpp"
#include "libfork/macro.hpp"

/**
 * @file task.hpp
 *
 * @brief The ``lf::task`` class.
 */

namespace lf {

namespace detail {

template <typename T, thread_context Context, tag Tag>
struct promise_type;

template <typename Head, typename... Tail>
struct invoker {

  using task_type = typename std::invoke_result_t<typename Head::underlying_async_fn, Head, Tail &&...>;
  using value_type = typename task_type::value_type;
  using promise_type = typename stdexp::coroutine_traits<task_type, Head, Tail &&...>::promise_type;
  using handle_type = typename stdexp::coroutine_handle<promise_type>;

  /**
   * @brief Invoke the stateless callable wrapped in Head with arguments head and tail...
   */
  static auto invoke(Head head, Tail &&...tail) -> handle_type {
    return handle_type::from_address(std::invoke(typename Head::underlying_async_fn{}, head, std::forward<Tail>(tail)...).m_handle);
  }
};

} // namespace detail

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T = void>
  requires(!std::is_reference_v<T>)
class task {
public:
  using value_type = T; ///< The type of the value returned by the coroutine (cannot be a reference, use ``std::reference_wrapper``).

private:
  explicit constexpr task(void *handle) noexcept : m_handle{handle} {}

  template <typename, thread_context, tag>
  friend struct detail::promise_type;

  template <typename, typename...>
  friend struct detail::invoker;

  void *m_handle;
};

} // namespace lf

#endif /* C9921D3E_28E4_4577_BB9C_E7CA55766E92 */
