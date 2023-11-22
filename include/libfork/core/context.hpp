#ifndef D66BBECE_E467_4EB6_B74A_AAA2E7256E02
#define D66BBECE_E467_4EB6_B74A_AAA2E7256E02

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>

#include "libfork/core/macro.hpp"
#include "libfork/core/tag.hpp"

#include "libfork/core/ext/deque.hpp"
#include "libfork/core/ext/handles.hpp"
#include "libfork/core/ext/list.hpp"

#include "libfork/core/impl/utility.hpp"

/**
 * @file meta.hpp
 *
 * @brief Provides the `thread_context` interface and meta programming utilities.
 */
namespace lf {

// ------------------ Context ------------------- //

class context; // User facing, (for submitting tasks).

inline namespace ext {

class worker_context; // API for worker threads.

} // namespace ext

namespace impl {

class full_context; // Internal API

} // namespace impl

/**
 * @brief Core-visible context for users to query and submit tasks to.
 */
class context : impl::immovable<context> {
 public:
  /**
   * @brief Construct a context for a worker thread.
   *
   * The lifetime of the stack source is expected to subsume the lifetime of the context.
   *
   * The `max_parallelism` is a user-interpreted hint for some algorithms. It is intended to represent
   * the maximum concurrent execution of a task forked by the worker owning this context.
   */
  explicit context(std::size_t max_parallelism) noexcept : m_max_parallelism(max_parallelism) {}

  /**
   * @brief Fetch the maximum amount of parallelism available to the user.
   *
   * This is a user-interpreted hint for some algorithms. It is intended to represent
   * the maximum concurrent execution of a task forked by the worker owning this context.
   */
  [[nodiscard]] constexpr auto max_parallelism() const noexcept -> std::size_t { return m_max_parallelism; }

  /**
   * @brief Submit pending/suspended tasks to the context.
   */
  void submit(intruded_list<submit_handle> jobs) noexcept { m_submit.push(jobs); }

 private:
  static constexpr std::size_t k_buff = 8;

  friend class worker_context;
  friend class impl::full_context;

  deque<task_handle> m_tasks;             ///< All non-null.
  intrusive_list<submit_handle> m_submit; ///< All non-null.

  std::size_t m_max_parallelism; ///< User interpreted hint for some algorithms.
};

inline namespace ext {

class worker_context : public context {
 public:
  using context::context;

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
