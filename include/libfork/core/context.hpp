#ifndef D66BBECE_E467_4EB6_B74A_AAA2E7256E02
#define D66BBECE_E467_4EB6_B74A_AAA2E7256E02

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <version>

#include "libfork/core/macro.hpp"
#include "libfork/core/tag.hpp"

#include "libfork/core/ext/deque.hpp"
#include "libfork/core/ext/handles.hpp"
#include "libfork/core/ext/list.hpp"

#include "libfork/core/impl/utility.hpp"

/**
 * @file context.hpp
 *
 * @brief Provides the hierarchy of worker thread contexts.
 */

namespace lf {

// ------------------ Context ------------------- //

class context; // User facing, (for submitting tasks).

inline namespace ext {

class worker_context; // API for worker threads.

} // namespace ext

namespace impl {

class full_context; // Internal API

struct switch_awaitable; // Forwadr decl for friend.

} // namespace impl

/**
 * @brief A type-erased function object that takes no arguments.
 */
#ifdef __cpp_lib_move_only_function
using nullary_function_t = std::move_only_function<void()>;
#else
using nullary_function_t = std::function<void()>;
#endif

/**
 * @brief Core-visible context for users to query and submit tasks to.
 */
class context : impl::immovable<context> {
 public:
  /**
   * @brief Construct a context for a worker thread.
   *
   * Notify is a function that may be called concurrently by other workers to signal to the worker
   * owning this context that a task has been submitted to a private queue.
   */
  explicit context(nullary_function_t notify) noexcept : m_notify(std::move(notify)) { LF_ASSERT(m_notify); }

 private:
  friend class ext::worker_context;
  friend class impl::full_context;
  friend struct impl::switch_awaitable;

  /**
   * @brief Submit pending/suspended tasks to the context.
   *
   * This is for use by implementor of the scheduler, this will trigger the notification function.
   */
  void submit(intruded_list<submit_handle> jobs) noexcept {
    m_submit.push(non_null(jobs));
    m_notify();
  }

  deque<task_handle> m_tasks;             ///< All non-null.
  intrusive_list<submit_handle> m_submit; ///< All non-null.

  nullary_function_t m_notify; ///< The user supplied notification function.
};

inline namespace ext {

class worker_context : public context {
 public:
  using context::context;

  using context::submit;

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   *
   * If there are no submitted tasks, then returned pointer will be null.
   */
  [[nodiscard]] auto try_pop_all() noexcept -> intruded_list<submit_handle> { return m_submit.try_pop_all(); }

  /**
   * @brief Attempt a steal operation from this contexts task deque.
   */
  [[nodiscard]] auto try_steal() noexcept -> steal_t<task_handle> { return m_tasks.steal(); }
};

} // namespace ext

namespace impl {

class full_context : public worker_context {
 public:
  using worker_context::worker_context;

  void push(task_handle task) noexcept { m_tasks.push(non_null(task)); }

  [[nodiscard]] auto pop() noexcept -> task_handle {
    return m_tasks.pop([]() -> task_handle {
      return nullptr;
    });
  }
};

} // namespace impl

} // namespace lf

#endif /* D66BBECE_E467_4EB6_B74A_AAA2E7256E02 */
