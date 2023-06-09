#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <thread>

#include "libfork/libfork.hpp"
#include "libfork/promise_base.hpp"
#include "libfork/queue.hpp"
#include "libfork/random.hpp"
#include "libfork/stack.hpp"
#include "libfork/thread_local.hpp"

/**
 * @file busy_pool.hpp
 *
 * @brief A work-stealing threadpool where all the threads spin when idle.
 */

namespace lf::detail {

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 */
class busy_pool {
public:
  /**
   * @brief The context type for the busy_pools threads.
   */
  class worker_context : thread_local_ptr<worker_context> {
  public:
    using stack_type = virtual_stack<4096>;

    static auto context() -> worker_context & { return get(); }

    constexpr auto max_threads() const noexcept -> std::size_t { return m_max_threads; }

    auto stack_top() -> stack_type::handle { return stack_type::handle{*m_stack}; }

    void stack_pop() {
      static_cast<void>(m_stack.release());
      m_stack = stack_type::make_unique();
    }

    void stack_push(stack_type::handle handle) {
      m_stack = unique_ptr{*handle};
    }

    auto task_steal() -> auto {
      return m_tasks.steal();
    }

    auto task_pop() -> std::optional<task_handle> {
      return m_tasks.pop();
    }

    void task_push(task_handle task) {
      m_tasks.push(task);
    }

  private:
    using unique_ptr = typename stack_type::unique_ptr_t;

    friend class busy_pool;

    std::size_t m_max_threads;
    unique_ptr m_stack = stack_type::make_unique();
    queue<task_handle> m_tasks;
    detail::xoshiro m_rng;
  };

  busy_pool(busy_pool const &) = delete;

  busy_pool(busy_pool &&) = delete;

  auto operator=(busy_pool const &) -> busy_pool & = delete;

  auto operator=(busy_pool &&) -> busy_pool & = delete;

  static_assert(thread_context<worker_context>);

  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {
    // Initialize the rngs.
    detail::xoshiro rng(std::random_device{});

    for (auto &ctx : m_contexts) {
      ctx.m_rng = rng;
      ctx.m_max_threads = n;
      rng.long_jump();
    }

    try {
      for (std::size_t i = 0; i < n; ++i) {
        m_workers.emplace_back([this, i]() {
          // Set the thread local context.
          worker_context::set(this->m_contexts[i]);

          for (;;) {
            // Wait for a root task to be submitted.
            LIBFORK_LOG("worker waits");

            this->m_root_task_in_flight.wait(false, std::memory_order_acquire);

            LIBFORK_LOG("worker wakes and starts stealing");

            if (auto submit = this->m_submit.exchange(detail::trivial_handle<void>{nullptr})) {
              LIBFORK_LOG("Gets root task");
              submit.resume();
            }

            this->steal_until(i, [&]() -> bool {
              return !m_root_task_in_flight.test(std::memory_order_acquire) || this->m_stop_requested.test(std::memory_order_acquire);
            });

            // If we have been awoken by the destructor, return.
            if (this->m_stop_requested.test(std::memory_order_acquire)) {
              LIBFORK_LOG("worker returns");
              return;
            }
          }
        });
      }
    } catch (...) {
      // Need to stop the threads
      clean_up();
      throw;
    }
  }

  /**
   * @brief Submit a task to the pool and join the workers until it completes.
   */

  auto schedule() {
    return [this](stdexp::coroutine_handle<> root) {
      //
      auto prev = m_submit.exchange(trivial_handle_impl<>{root});

      LIBFORK_ASSERT(!prev);

      LIBFORK_LOG("waking workers");

      // Wake up the workers.
      m_root_task_in_flight.test_and_set(std::memory_order_release);
      m_root_task_in_flight.notify_all();
    };
  }

  ~busy_pool() noexcept { clean_up(); }

private:
  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {
    LIBFORK_ASSUME(!m_root_task_in_flight.test());

    // Set conditions for workers to stop
    m_stop_requested.test_and_set(std::memory_order_release);

    // Wake workers waiting on m_root_task_in_flight
    m_root_task_in_flight.test_and_set(std::memory_order_release);
    m_root_task_in_flight.notify_all();

    // Join workers
    for (auto &worker : m_workers) {
      LIBFORK_ASSUME(worker.joinable());
      worker.join();
    }
  }

  static constexpr std::size_t k_steal_attempts = 1024;

  alignas(detail::k_cache_line) std::atomic_flag m_stop_requested = ATOMIC_FLAG_INIT;
  alignas(detail::k_cache_line) std::atomic_flag m_root_task_in_flight = ATOMIC_FLAG_INIT;

  std::atomic<detail::trivial_handle<>> m_submit;
  std::vector<worker_context> m_contexts;
  std::vector<std::thread> m_workers; // After m_context so threads are destroyed before the queues.

  template <typename F>
  void steal_until(std::size_t uid, F &&cond) {
    //
    LIBFORK_ASSUME(uid < m_contexts.size());

    auto &my_context = m_contexts[uid];

    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

    while (!cond()) {
      std::size_t attempt = 0;

      while (attempt < k_steal_attempts) {
        if (std::size_t const steal_at = dist(my_context.m_rng); steal_at != uid) {
          if (auto work = m_contexts[steal_at].task_steal()) {
            attempt = 0;
            LIBFORK_LOG("resuming stolen work");
            work->resume();
            LIBFORK_LOG("worker resumes thieving");
            LIBFORK_ASSUME(my_context.m_tasks.empty());
            LIBFORK_ASSUME(my_context.m_stack->empty());
          } else {
            ++attempt;
          }
        }
      }
    };
  }
};

} // namespace lf::detail

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
