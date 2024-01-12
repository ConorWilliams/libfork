#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>  // for atomic_flag, memory_or...
#include <cstddef> // for size_t
#include <latch>   // for latch
#include <memory>  // for shared_ptr, __shared_p...
#include <random>  // for random_device, uniform...
#include <span>    // for span
#include <thread>  // for thread
#include <utility> // for move
#include <vector>  // for vector

#include "libfork/core/defer.hpp"                 // for LF_DEFER
#include "libfork/core/ext/context.hpp"           // for context, nullary_funct...
#include "libfork/core/ext/handles.hpp"           // for submit_handle, task_ha...
#include "libfork/core/ext/list.hpp"              // for for_each_elem, intrude...
#include "libfork/core/ext/resume.hpp"            // for resume
#include "libfork/core/impl/utility.hpp"          // for k_cache_line, move_only
#include "libfork/core/macro.hpp"                 // for LF_ASSERT, LF_ASSERT_N...
#include "libfork/core/sync_wait.hpp"             // for scheduler
#include "libfork/schedule/ext/numa.hpp"          // for numa_strategy, numa_to...
#include "libfork/schedule/ext/random.hpp"        // for xoshiro, seed
#include "libfork/schedule/impl/numa_context.hpp" // for numa_context

/**
 * @file busy_pool.hpp
 *
 * @brief A work-stealing thread pool where all the threads spin when idle.
 */

namespace lf {

namespace impl {

/**
 * @brief Variable used to synchronize a collection of threads.
 */
struct busy_vars {
  /**
   * @brief Construct a new busy vars object for synchronizing `n` workers with one master.
   */
  explicit busy_vars(std::size_t n) : latch_start(n + 1), latch_stop(n) { LF_ASSERT(n > 0); }

  alignas(k_cache_line) std::latch latch_start; ///< Synchronize construction.
  alignas(k_cache_line) std::latch latch_stop;  ///< Synchronize destruction.
  alignas(k_cache_line) std::atomic_flag stop;  ///< Signal shutdown.
};

/**
 * @brief Workers event-loop.
 */
inline void busy_work(numa_topology::numa_node<impl::numa_context<busy_vars>> node) noexcept {

  LF_ASSERT(!node.neighbors.empty());
  LF_ASSERT(!node.neighbors.front().empty());

  // ------- Initialize my numa variables

  std::shared_ptr my_context = node.neighbors.front().front();

  my_context->init_worker_and_bind(nullary_function_t{[]() {}}, node); // Notification is a no-op.

  // Wait for everyone to have set up their numa_vars. If this throws an exception then
  // program terminates due to the noexcept marker.
  my_context->shared().latch_start.arrive_and_wait();

  LF_DEFER {
    // Wait for everyone to have stopped before destroying the context (which others could be observing).
    my_context->shared().stop.test_and_set(std::memory_order_release);
    my_context->shared().latch_stop.arrive_and_wait();
    my_context->finalize_worker();
  };

  // -------

  while (!my_context->shared().stop.test(std::memory_order_acquire)) {

    intruded_list<submit_handle> submissions = my_context->try_pop_all();

    for_each_elem(submissions, [](lf::submit_handle submitted) LF_STATIC_CALL noexcept {
      resume(submitted);
    });

    if (task_handle task = my_context->try_steal()) {
      resume(task);
    }
  };
}

} // namespace impl

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 * Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class busy_pool : impl::move_only<busy_pool> {

  std::size_t m_num_threads;
  std::uniform_int_distribution<std::size_t> m_dist{0, m_num_threads - 1};
  xoshiro m_rng{seed, std::random_device{}};
  std::shared_ptr<impl::busy_vars> m_share = std::make_shared<impl::busy_vars>(m_num_threads);
  std::vector<std::shared_ptr<impl::numa_context<impl::busy_vars>>> m_worker = {};
  std::vector<std::thread> m_threads = {};
  std::vector<worker_context *> m_contexts = {};

 public:
  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   * @param strategy The numa strategy for distributing workers.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency(),
                     numa_strategy strategy = numa_strategy::fan)
      : m_num_threads(n) {

    for (std::size_t i = 0; i < n; ++i) {
      m_worker.push_back(std::make_shared<impl::numa_context<impl::busy_vars>>(m_rng, m_share));
      m_rng.long_jump();
    }

    LF_ASSERT_NO_ASSUME(!m_share->stop.test(std::memory_order_acquire));

    std::vector nodes = numa_topology{}.distribute(m_worker, strategy);

    [&]() noexcept {
      // All workers must be created, if we fail to create them all then we must terminate else
      // the workers will hang on the start latch.
      for (auto &&node : nodes) {
        m_threads.emplace_back(impl::busy_work, std::move(node));
      }

      // Wait for everyone to have set up their numa_vars before submitting. This
      // must be noexcept as if we fail the countdown then the workers will hang.
      m_share->latch_start.arrive_and_wait();
    }();

    // All workers have set their contexts, we can read them now.
    for (auto &&worker : m_worker) {
      m_contexts.push_back(worker->get_underlying());
    }
  }

  /**
   * @brief Schedule a task for execution.
   */
  void schedule(lf::intruded_list<lf::submit_handle> jobs) { m_worker[m_dist(m_rng)]->submit(jobs); }

  /**
   * @brief Get a view of the worker's contexts.
   */
  auto contexts() noexcept -> std::span<worker_context *> { return m_contexts; }

  ~busy_pool() noexcept {
    LF_LOG("Requesting a stop");
    // Set conditions for workers to stop
    m_share->stop.test_and_set(std::memory_order_release);

    for (auto &worker : m_threads) {
      worker.join();
    }
  }
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
