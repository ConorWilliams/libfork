#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <random>
#include <thread>

#include "libfork/libfork.hpp"
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
    using stack_type = virtual_stack<4096>;

    static auto context() -> context_type & { return get(); }

    constexpr auto max_threads() const noexcept -> std::size_t { return m_max_threads; }

    auto stack_top() -> stack_type::handle { return stack_type::handle{*m_stack}; }

    void stack_pop() {
      static_cast<void>(m_stack.release());
      m_stack = stack_type::make_unique();
    }

    void stack_push(stack_type::handle handle) {
      m_stack = unique_ptr{*handle};
    }

    auto task_steal() -> auto{
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
          // Set the thread local context.
          context_type::set(this->m_contexts[i]);

          for (;;) {
            // Wait for a root task to be submitted.
            LIBFORK_LOG("worker waits");

            this->m_root_task_in_flight.wait(false, std::memory_order_acquire);

            LIBFORK_LOG("worker wakes and starts stealing");

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
#if LIBFORK_COMPILER_EXCEPTIONS
    } catch (...) {
      // Need to stop the threads
      clean_up();
      throw;
    }
#endif
  }

  /**
   * @brief Submit a task to the pool and join the workers until it completes.
   */
  template <stateless Fn, class... Args>
  auto schedule(Fn fun, Args &&...args) {

    auto submit = [this](stdexp::coroutine_handle<> root) {
      //
      auto prev = m_submit.exchange(stdexp::coroutine_handle<>{root});

      LIBFORK_LOG("waking workers");

      // Wake up the workers.
      m_root_task_in_flight.test_and_set(std::memory_order_release);
      m_root_task_in_flight.notify_all();
    };

    auto res = sync_wait(submit, fun, std::forward<Args>(args)...);

    m_root_task_in_flight.clear(std::memory_order_release);

    return res;
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

  std::atomic_flag m_stop_requested = ATOMIC_FLAG_INIT;
  std::atomic_flag m_root_task_in_flight = ATOMIC_FLAG_INIT;

  std::barrier m_barrier;

  std::atomic<stdexp::coroutine_handle<>> m_submit;
  std::vector<context_type> m_contexts;
  std::vector<std::thread> m_workers; // After m_context so threads are destroyed before the queues.

  template <typename F>
  void steal_until(std::size_t uid, F &&cond) {
    //
    LIBFORK_ASSUME(uid < m_contexts.size());

    auto &my_context = m_contexts[uid];

    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

    while (!cond()) {
      std::size_t attempt = 0;

      if (auto submit = this->m_submit.exchange(stdexp::coroutine_handle<>{nullptr})) {
        LIBFORK_LOG("Gets root task");
        submit.resume();
      }

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

    if (auto submit = this->m_submit.exchange(stdexp::coroutine_handle<>{nullptr})) {
      LIBFORK_LOG("Gets root task");
      submit.resume();
    }
  }
};

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
