
#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <optional>
#include <random>
#include <thread>

#include "libfork/detail/random.hpp"
#include "libfork/detail/utility.hpp"

#include "libfork/event_count.hpp"
#include "libfork/queue.hpp"
#include "libfork/task.hpp"

/**
 * @file thread_pool.hpp
 *
 * @brief A work-stealing scheduler that sleeps idle workers.
 */

namespace lf {

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 */
class thread_pool {
 public:
  /**
   * @brief The context type for the thread_pools threads.
   */
  class context : private queue<work_handle<context>> {
   public:
    using queue<work_handle<context>>::push;
    using queue<work_handle<context>>::pop;

   private:
    friend class thread_pool;

    detail::xoshiro m_rng;
  };

  static_assert(::lf::context<context>);

  thread_pool(thread_pool const&) = delete;

  thread_pool(thread_pool&&) = delete;

  auto operator=(thread_pool const&) -> thread_pool& = delete;

  auto operator=(thread_pool&&) -> thread_pool& = delete;

  ~thread_pool() noexcept { clean_up(); }

  /**
   * @brief Construct a new thread_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit thread_pool(std::size_t n = std::thread::hardware_concurrency());
  /**
   * @brief Submit a task to the pool and join the workers until it completes.
   */
  template <typename T, typename Allocator, bool Root>
  auto schedule(basic_task<T, context, Allocator, Root>&& task) -> T;

 private:
  static constexpr std::size_t k_steal_attempts = 1024;

  //  m_stop, m_contexts, m_workers, m_active, m_thieves, m_notify, m_submit

  alignas(detail::k_cache_line) std::atomic_flag m_stop = ATOMIC_FLAG_INIT;

  std::vector<context> m_contexts;
  std::vector<std::thread> m_workers;

  alignas(detail::k_cache_line) std::atomic_int m_active;
  alignas(detail::k_cache_line) std::atomic_int m_thieves;

  event_count m_notify;

  queue<root_handle<context>> m_submit;

  // Work until the condition is met, Can wait if wait is true
  template <bool Root>
  auto exploit_task(std::size_t uid, task_handle<context, Root> task) -> void;

  auto wait_for_task(std::size_t uid, std::optional<work_handle<context>>& task) -> bool;

  auto try_steal_task(std::size_t uid) -> std::optional<work_handle<context>>;

  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void;
};

// ---- Implementation ----

inline thread_pool::thread_pool(std::size_t n) : m_contexts(n) {
  // Initialize the rngs.
  detail::xoshiro rng(std::random_device{});

  for (auto& ctx : m_contexts) {
    ctx.m_rng = rng;
    rng.long_jump();
  }

  try {
    // Start the worker threads, note there are n workers, indexed 0...n - 1.
    for (std::size_t i = 0; i < n; ++i) {
      m_workers.emplace_back([this, i]() {
        //
        while (true) {
          m_thieves.fetch_add(1, std::memory_order_release);

          if (std::optional task = try_steal_task(i)) {
            if (m_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1) {
              m_notify.notify_one();
            }
            exploit_task(i, *task);
            continue;
          }

          auto key = m_notify.prepare_wait();

          if (auto root = m_submit.steal()) {
            m_notify.cancel_wait();
            if (m_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1) {
              m_notify.notify_one();
            }
            exploit_task(i, *root);
            continue;
          }

          if (m_stop.test(std::memory_order_acquire)) {
            m_notify.cancel_wait();
            m_notify.notify_all();
            m_thieves.fetch_sub(1, std::memory_order_release);
            DEBUG_TRACKER("worker exits");
            return;
          }

          if (m_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1 && m_active.load(std::memory_order_acquire) > 0) {
            m_notify.cancel_wait();
            continue;
          }

          DEBUG_TRACKER("bedtime");

          m_notify.wait(key);

          DEBUG_TRACKER("rise and shine");
        }
      });
    }
  } catch (...) {
    clean_up();
    throw;
  }
}

template <typename T, typename Allocator, bool Root>
auto thread_pool::schedule(basic_task<T, context, Allocator, Root>&& task) -> T {
  //
  auto [fut, handle] = as_root(std::move(task)).make_promise();

  m_submit.push(handle);

  m_notify.notify_one();

  fut.wait();

  if constexpr (!std::is_void_v<T>) {
    return *std::move(fut);
  } else {
    return;
  }
}

template <bool Root>
inline auto thread_pool::exploit_task(std::size_t uid, task_handle<context, Root> task) -> void {
  //
  if (m_active.fetch_add(1, std::memory_order_acq_rel) == 0 && m_thieves.load(std::memory_order_acquire) == 0) {
    m_notify.notify_one();
  }

  task.resume(m_contexts[uid]);

  m_active.fetch_sub(1, std::memory_order_release);
}

inline auto thread_pool::try_steal_task(std::size_t uid) -> std::optional<work_handle<context>> {
  //
  auto& my_context = m_contexts[uid];

  std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

  for (std::size_t i = 0; i < k_steal_attempts; ++i) {
    if (std::size_t const steal_at = dist(my_context.m_rng); steal_at != uid) {
      if (auto work = m_contexts[steal_at].steal()) {
        return {*work};
      }
    }
  }
  return std::nullopt;
}

inline auto thread_pool::clean_up() noexcept -> void {
  // Set conditions for workers to stop
  m_stop.test_and_set(std::memory_order_release);
  DEBUG_TRACKER("waking workers to terminate");
  m_notify.notify_all();

  // Join workers
  for (auto& worker : m_workers) {
    DEBUG_TRACKER("joining worker");
    ASSERT_ASSUME(worker.joinable(), "this should be impossible");
    worker.join();
  }
}

}  // namespace lf