
#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <random>
#include <thread>

#include "libfork/detail/random.hpp"
#include "libfork/detail/utility.hpp"

#include "libfork/queue.hpp"
#include "libfork/task.hpp"

/**
 * @file busy_pool.hpp
 *
 * @brief A traditional work-stealing scheduler.
 */

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
  class context : private queue<work_handle<context>> {
   public:
    using queue<work_handle<context>>::push;
    using queue<work_handle<context>>::pop;

   private:
    friend class busy_pool;

    detail::xoshiro m_rng;
  };

  busy_pool(busy_pool const&) = delete;

  busy_pool(busy_pool&&) = delete;

  auto operator=(busy_pool const&) -> busy_pool& = delete;

  auto operator=(busy_pool&&) -> busy_pool& = delete;

  static_assert(::lf::context<context>);

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

    try {
      // Start the worker threads, note there are n-1 workers, indexed 1...n - 1.
      for (std::size_t i = 1; i < n; ++i) {
        m_workers.emplace_back([this, i]() {  // NOLINT
          for (;;) {
            // Wait for a root task to be submitted.
            DEBUG_TRACKER("worker waits");

            this->m_root_task_in_flight.wait(false, std::memory_order_acquire);

            DEBUG_TRACKER("worker wakes and starts stealing");

            this->steal_until(i, [&]() -> bool {
              return !m_root_task_in_flight.test(std::memory_order_acquire) || this->m_stop_requested.test(std::memory_order_acquire);
            });

            // If we have been awoken by the destructor, return.
            if (this->m_stop_requested.test(std::memory_order_acquire)) {
              DEBUG_TRACKER("worker returns");
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
  template <typename T, typename Allocator, bool Root>
  auto schedule(basic_task<T, context, Allocator, Root>&& task) -> T {
    //
    constexpr std::size_t uid = 0;

    auto [fut, handle] = as_root(std::move(task)).make_promise();

    DEBUG_TRACKER("waking workers");

    // Wake up the workers.
    m_root_task_in_flight.test_and_set(std::memory_order_release);
    m_root_task_in_flight.notify_all();

    DEBUG_TRACKER("root task starts");

    // Start the root task.
    handle.resume(m_contexts[uid]);

    DEBUG_TRACKER("master thread starts stealing");

    // Steal until root task completes, need doggy capture until clang implements P1091
    steal_until(uid, [fut = std::addressof(fut)]() -> bool {
      return fut->is_ready();
    });

    DEBUG_TRACKER("master thread returns");

    m_root_task_in_flight.clear(std::memory_order_release);

    if constexpr (!std::is_void_v<T>) {
      return *std::move(fut);
    } else {
      return;
    }
  }

  ~busy_pool() noexcept { clean_up(); }

 private:
  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {
    ASSERT_ASSUME(!m_root_task_in_flight.test(), "clean-up with work in-flight");

    // Set conditions for workers to stop
    m_stop_requested.test_and_set(std::memory_order_release);

    // Wake workers waiting on m_root_task_in_flight
    m_root_task_in_flight.test_and_set(std::memory_order_release);
    m_root_task_in_flight.notify_all();

    // Join workers
    for (auto& worker : m_workers) {
      ASSERT_ASSUME(worker.joinable(), "this should be impossible");
      worker.join();
    }
  }

  static constexpr std::size_t k_steal_attempts = 1024;

  alignas(detail::k_cache_line) std::atomic_flag m_stop_requested = ATOMIC_FLAG_INIT;
  alignas(detail::k_cache_line) std::atomic_flag m_root_task_in_flight = ATOMIC_FLAG_INIT;

  std::vector<context> m_contexts;
  std::vector<std::thread> m_workers;  // After m_context so threads are destroyed before the queues.

  template <typename F>
  void steal_until(std::size_t uid, F&& cond) {
    //
    ASSERT_ASSUME(uid < m_contexts.size(), "bad uid");

    auto& my_context = m_contexts[uid];

    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

    while (!cond()) {
      std::size_t attempt = 0;

      while (attempt < k_steal_attempts) {
        if (std::size_t const steal_at = dist(my_context.m_rng); steal_at != uid) {
          if (auto work = m_contexts[steal_at].steal()) {
            attempt = 0;
            DEBUG_TRACKER("resuming stolen work");
            work->resume(my_context);
            DEBUG_TRACKER("worker resumes thieving");
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