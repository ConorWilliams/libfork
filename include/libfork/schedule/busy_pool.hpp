#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <atomic>
#include <bit>
#include <memory>
#include <random>
#include <thread>

#include "libfork/core.hpp"

#include "libfork/schedule/contexts.hpp"
#include "libfork/schedule/numa.hpp"
#include "libfork/schedule/random.hpp"

/**
 * @file busy_pool.hpp
 *
 * @brief A work-stealing thread pool where all the threads spin when idle.
 */

namespace lf {

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 * Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class busy_pool {
 public:
  struct context_type : impl::numa_worker_context<context_type> {
    using numa_worker_context::numa_worker_context;
  };

 private:
  xoshiro m_rng{seed, std::random_device{}};

  std::vector<std::shared_ptr<context_type>> m_contexts;
  std::vector<std::thread> m_workers;
  std::unique_ptr<std::atomic_flag> m_stop = std::make_unique<std::atomic_flag>();

  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {

    LF_LOG("Requesting a stop");
    // Set conditions for workers to stop
    m_stop->test_and_set(std::memory_order_release);

    for (auto &worker : m_workers) {
      worker.join();
    }
  }

  static auto work(numa_topology::numa_node<context_type> node, std::atomic_flag const &stop_requested) {

    std::shared_ptr my_context = node.neighbors.front().front();

    worker_init(my_context.get());

    impl::defer at_exit = [&]() noexcept {
      worker_finalize(my_context.get());
    };

    my_context->init_numa_and_bind(node);

    while (!stop_requested.test(std::memory_order_acquire)) {

      for_each(my_context->try_get_submitted(), [](submit_h<context_type> *submitted) LF_STATIC_CALL noexcept {
        resume(submitted);
      });

      if (auto *task = my_context->try_steal()) {
        resume(task);
      }
    };
  }

 public:
  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) {

    for (std::size_t i = 0; i < n; ++i) {
      m_contexts.push_back(std::make_shared<context_type>(n, m_rng));
      m_rng.long_jump();
    }

    LF_ASSERT_NO_ASSUME(!m_stop->test(std::memory_order_acquire));

    std::vector nodes = numa_topology{}.distribute(m_contexts);

    // clang-format off

    LF_TRY {
      for (auto &&node : nodes) {
        m_workers.emplace_back(work, std::move(node), std::cref(*m_stop));
      }
    } LF_CATCH_ALL {
      clean_up();
      LF_RETHROW;
    }

    // clang-format on
  }

  ~busy_pool() noexcept { clean_up(); }

  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(intruded_h<context_type> *node) noexcept {
    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);
    m_contexts[dist(m_rng)]->submit(node);
  }
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
