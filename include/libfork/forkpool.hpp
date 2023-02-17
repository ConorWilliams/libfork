
#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <optional>
#include <random>
#include <semaphore>
#include <stop_token>
#include <thread>

#include "libfork/queue.hpp"
#include "libfork/random.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

namespace lf {

/**
 * @brief a handle to a team
 *
 */
class forkpool {
 public:
  /**
   * @brief The context type for the forkpools threads.
   */
  struct context : queue<task_handle<context>> {};
  /**
   * @brief Construct a new forkpool object.
   *
   * @param n
   */
  explicit forkpool(std::size_t n = std::thread::hardware_concurrency())
      : m_contexts(n), m_workers(n) {
    //
    ASSERT_ASSUME(n > 0, "Threadpool must have at least one thread.");

    xoshiro rng(std::random_device{});

    for (std::size_t id = 0; id < n; ++id) {
      m_workers.emplace_back([this, id, rng, n](std::stop_token token) mutable {  // NOLINT
        // Initialize PRNG stream
        for (size_t j = 0; j < id; j++) {
          rng.long_jump();
        }

        context& my_context = this->m_contexts[id];

        std::uniform_int_distribution<std::size_t> dist(0, n - 1);

        while (!token.stop_requested()) {
          if (auto work = steal(m_submission_queue)) {
            DEBUG_TRACKER("resuming work from submission queue");
            work->resume_root(my_context);
            ASSERT_ASSUME(my_context.empty(), "should have no work left");
          }
          for (std::size_t i = 0; i < n * n * n; ++i) {
            if (std::size_t steal_at = dist(rng); steal_at != id) {
              if (auto work = steal(m_contexts[steal_at])) {
                DEBUG_TRACKER("resuming stolen work");
                work->resume_stolen(my_context);
                ASSERT_ASSUME(my_context.empty(), "should have no work left");
              }
            }
          }
        }
      });
    }
  }

  /**
   * @brief do the thing
   *
   * @tparam T
   * @param work
   * @return T
   */
  template <typename T>
  auto sync_wait(task<T, context> work) -> T {
    //
    std::binary_semaphore sem{0};

    future<T> res;

    task root_task = this->make<T>(work, sem, res);

    m_submission_queue.push(root_task.get_handle());

    sem.acquire();

    DEBUG_TRACKER("semaphore acquired");

    return res;
  }

 private:
  /**
   * @brief Attempt to steal work until ``queue`` is empty.
   *
   * @param queue
   * @return std::optional<context>
   */
  static auto steal(queue<task_handle<context>>& queue) -> std::optional<task_handle<context>> {
    //
    auto [err, work] = queue.steal();

    if (err == err::none) {
      return work;
    }

    return std::nullopt;
  }

  template <typename T>
  static auto make(auto work, std::binary_semaphore& sem, future<T>& res) -> task<void, context> {
    co_await just(res, work);
    DEBUG_TRACKER("semaphore released");
    sem.release();
  };

  queue<task_handle<context>> m_submission_queue;

  std::vector<context> m_contexts;

  std::vector<std::jthread> m_workers;
};

}  // namespace lf