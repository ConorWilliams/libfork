
#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <optional>
#include <random>
#include <stop_token>
#include <thread>

#include "libfork/detail/random.hpp"
#include "libfork/queue.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

namespace lf {

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
  class context : private queue<task_handle<context>> {
   public:
    // using queue<task_handle<context>>::push;
    // using queue<task_handle<context>>::pop;

    /**
     * @brief Push a task onto the queue.
     */
    auto push(task_handle<context> task) -> void {
      ASSERT(m_id == std::this_thread::get_id(), "push accessed from wrong thread");
      queue<task_handle<context>>::push(task);
    }
    /**
     * @brief Pop a task from the queue.
     */
    auto pop() -> std::optional<task_handle<context>> {
      ASSERT(m_id == std::this_thread::get_id(), "pop accessed from wrong thread");
      return queue<task_handle<context>>::pop();
    }

   private:
    friend class busy_pool;

    detail::xoshiro m_rng;

#ifndef NDEBUG
    std::thread::id m_id;
#endif
  };

  busy_pool(busy_pool const&) = delete;

  busy_pool(busy_pool&&) = delete;

  auto operator=(busy_pool const&) -> busy_pool& = delete;

  auto operator=(busy_pool&&) -> busy_pool& = delete;

  static_assert(::lf::context<context>);

  /**
   * @brief The number of steals attempted before worker polls its stop condition.
   */
  static constexpr std::size_t steal_attempts = 1024;

  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {
    // Initialize the rngs.
    detail::xoshiro rng(std::random_device{});

    for (auto& ctx : m_contexts) {
      ctx.m_rng = rng;
      rng.long_jump();
    }

#ifndef NDEBUG
    m_contexts[0].m_id = std::this_thread::get_id();
#endif

    // Start the worker threads, note there are n-1 workers, indexed 1...n - 1.
    for (std::size_t i = 1; i < n; ++i) {
      m_workers.emplace_back([this, i, n](std::stop_token token) {  // NOLINT
//
#ifndef NDEBUG
        this->m_contexts[i].m_id = std::this_thread::get_id();
#endif
        for (;;) {
          // Wait for a root task to be submitted.
          this->m_root_task_in_flight.wait(false, std::memory_order_acquire);

          DEBUG_TRACKER("worker wakes");

          // If we have been awoken by the destructor, return.
          if (token.stop_requested()) {
            DEBUG_TRACKER("worker returns");
            return;
          }

          DEBUG_TRACKER("worker works");

          steal_until(i, [&]() -> bool {
            return !m_root_task_in_flight.test(std::memory_order_acquire) || token.stop_requested();
          });
        }
      });
    }
  }

  /**
   * @brief Submit a task to the pool and join the workers until it completes.
   */
  template <typename T, typename Allocator>
  auto schedule(basic_task<T, context, Allocator>&& task) -> T {
    //
    constexpr std::size_t uid = 0;
    //
#ifndef NDEBUG
    ASSERT(this->m_contexts[uid].m_id == std::this_thread::get_id(), "root task accessed from wrong thread");
#endif
    //
    auto [fut, handle] = make_root(std::move(task)).make_promise();

    DEBUG_TRACKER("waking workers");

    // Wake up the workers
    m_root_task_in_flight.test_and_set(std::memory_order_release);
    m_root_task_in_flight.notify_all();

    DEBUG_TRACKER("root task starts");

    // Start the root task
    handle.resume_root(m_contexts[uid]);

    DEBUG_TRACKER("master thread starts stealing");

    // Steal until root task completes
    steal_until(uid, [&]() -> bool {
      return !m_root_task_in_flight.test(std::memory_order_acquire);
    });

    if constexpr (!std::is_void_v<T>) {
      return *std::move(fut);
    } else {
      return;
    }
  }

  ~busy_pool() {
    // Request all workers to stop.
    for (auto& worker : m_workers) {
      worker.request_stop();
    }
    // Wake up the workers so they can see the stop request.
    m_root_task_in_flight.test_and_set(std::memory_order_release);
    m_root_task_in_flight.notify_all();
  }

 private:
  std::atomic_flag m_root_task_in_flight = ATOMIC_FLAG_INIT;
  std::vector<context> m_contexts;
  std::vector<std::jthread> m_workers;  // After m_context so threads are destroyed before the queues.

  template <typename T, typename Allocator>
  auto make_root(basic_task<T, context, Allocator>&& task) -> basic_task<T, context, Allocator> {
    defer on_exit = [this]() noexcept {
      DEBUG_TRACKER("root task completes");
      this->m_root_task_in_flight.clear(std::memory_order_release);
    };
    co_return co_await std::move(task);
  }

  template <typename F>
  void steal_until(std::size_t uid, F&& cond) {
    //
    ASSERT_ASSUME(uid < m_contexts.size(), "bad uid");

    auto& my_context = m_contexts[uid];

    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

    while (!cond()) {
      std::size_t attempt = 0;

      while (attempt < steal_attempts) {
        if (std::size_t const steal_at = dist(my_context.m_rng); steal_at != uid) {
          if (auto work = m_contexts[steal_at].steal()) {
            attempt = 0;
            DEBUG_TRACKER("resuming stolen work");
            work->resume_stolen(my_context);
            ASSERT_ASSUME(my_context.empty(), "should have no work left");
          } else {
            ++attempt;
          }
        }
      }
    };
  }
};

}  // namespace lf