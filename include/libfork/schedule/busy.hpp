#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <random>
#include <thread>

#include "libfork/core/stack.hpp"
#include "libfork/event_count.hpp"
#include "libfork/libfork.hpp"
#include "libfork/macro.hpp"
#include "libfork/queue.hpp"
#include "libfork/random.hpp"
#include "libfork/thread_local.hpp"

/**
 * @file busy.hpp
 *
 * @brief A work-stealing threadpool where all the threads spin when idle.
 */

namespace lf {

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 */
class busy_pool : detail::immovable {
public:
  /**
   * @brief The context type for the busy_pools threads.
   */
  class context_type : thread_local_ptr<context_type> {
  public:
    using stack_type = virtual_stack<detail::mebibyte>;

    static auto context() -> context_type & { return get(); }

    constexpr auto max_threads() const noexcept -> std::size_t { return m_max_threads; }

    auto stack_top() -> stack_type::handle {
      LIBFORK_ASSERT(&context() == this);
      return stack_type::handle{*m_stack};
    }

    void stack_pop() {
      LIBFORK_ASSERT(&context() == this);
      static_cast<void>(m_stack.release());
      m_stack = stack_type::make_unique();
    }

    void stack_push(stack_type::handle handle) {
      LIBFORK_ASSERT(&context() == this);
      m_stack = unique_ptr{*handle};
    }

    auto task_steal() -> typename queue<task_handle>::steal_t {
      return m_tasks.steal();
    }

    auto task_pop() -> std::optional<task_handle> {
      LIBFORK_ASSERT(&context() == this);
      return m_tasks.pop();
    }

    void task_push(task_handle task) {
      LIBFORK_ASSERT(&context() == this);
      m_tasks.push(task);
    }

  private:
    using unique_ptr = typename stack_type::unique_ptr_t;

    friend class busy_pool;

    std::size_t m_max_threads = 0;
    unique_ptr m_stack = stack_type::make_unique();
    queue<task_handle> m_tasks;
    xoshiro m_rng;
  };

  static_assert(thread_context<context_type>);

  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {
    // Initialize the random number generator.
    xoshiro rng(std::random_device{});

    for (auto &ctx : m_contexts) {
      ctx.m_rng = rng;
      ctx.m_max_threads = n;
      rng.long_jump();
    }

#if LIBFORK_COMPILER_EXCEPTIONS
    try {
#endif
      for (std::size_t i = 0; i < n; ++i) {
        m_workers.emplace_back([this, i]() {
          // Get a reference to the threads context.
          context_type &my_context = m_contexts[i];

          // Set the thread local context.
          context_type::set(my_context);

          std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

          while (!m_stop_requested.test(std::memory_order_acquire)) {

            for (int attempt = 0; attempt < k_steal_attempts; ++attempt) {

              std::size_t steal_at = dist(my_context.m_rng);

              if (steal_at == i) {
                if (auto root = m_submit.steal()) {
                  LIBFORK_LOG("resuming root task");
                  root->resume();
                }
              } else if (auto work = m_contexts[steal_at].task_steal()) {
                attempt = 0;
                LIBFORK_LOG("resuming stolen work");
                work->resume();
                LIBFORK_LOG("worker resumes thieving");
                LIBFORK_ASSUME(my_context.m_tasks.empty());
                LIBFORK_ASSUME(my_context.m_stack->empty());
              }
            }
          };

          LIBFORK_LOG("Worker {} stopping", i);
        });
      }
#if LIBFORK_COMPILER_EXCEPTIONS
    } catch (...) {
      // Need to stop the threads
      clean_up();
      throw;
    }
#endif
  }

  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(stdx::coroutine_handle<> root) noexcept {
    m_submit.push(root);
  }

  ~busy_pool() noexcept { clean_up(); }

private:
  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {

    LIBFORK_LOG("Request stop");

    LIBFORK_ASSERT(m_submit.empty());

    // Set conditions for workers to stop
    m_stop_requested.test_and_set(std::memory_order_release);

    // Join workers
    for (auto &worker : m_workers) {
      LIBFORK_ASSUME(worker.joinable());
      worker.join();
    }
  }

  static constexpr int k_steal_attempts = 1024;

  queue<stdx::coroutine_handle<>> m_submit;

  std::atomic_flag m_stop_requested = ATOMIC_FLAG_INIT;

  std::vector<context_type> m_contexts;
  std::vector<std::thread> m_workers; // After m_context so threads are destroyed before the queues.
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
