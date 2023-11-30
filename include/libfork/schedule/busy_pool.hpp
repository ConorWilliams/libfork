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
#include <cstddef>
#include <latch>
#include <memory>
#include <numeric>
#include <random>
#include <thread>

#include "libfork/core.hpp"

#include "libfork/schedule/ext/numa.hpp"
#include "libfork/schedule/ext/random.hpp"

// #include "libfork/schedule/impl/contexts.hpp"

/**
 * @file busy_pool.hpp
 *
 * @brief A work-stealing thread pool where all the threads spin when idle.
 */

namespace lf {

namespace impl {

struct numa_vars {
 private:
  static constexpr std::size_t k_buff = 16;
  static constexpr std::size_t k_steal_attempts = 32; ///< Attempts per target.

  xoshiro m_rng;
  worker_context *m_context = nullptr;
  std::vector<std::vector<numa_vars *>> m_neigh;

 public:
  explicit numa_vars(xoshiro const &rng) : m_rng(rng) {}

  void submit(lf::intruded_list<lf::submit_handle> jobs) {
    LF_ASSERT(m_context);
    m_context->submit(jobs);
  }

  /**
   * @brief Initialize the context with the given topology and bind it to a thread.
   *
   * This is separate from construction as the master thread will need to construct
   * the contexts before they can form a reference to them, this must be called by the
   * worker thread.
   *
   * The context will store __raw__ pointers to the other contexts in the topology, this is
   * to ensure no circular references are formed.
   */
  void init_numa_and_bind(worker_context *context, numa_topology::numa_node<numa_vars> const &topo) {

    LF_ASSERT(!topo.neighbors.empty());
    LF_ASSERT(!topo.neighbors.front().empty());
    LF_ASSERT(topo.neighbors.front().front().get() == this);

    m_context = context;

    topo.bind();

    // Skip the first one as it is just us.
    for (std::size_t i = 1; i < topo.neighbors.size(); ++i) {
      m_neigh.push_back(map(topo.neighbors[i], [](std::shared_ptr<numa_vars> const &context) {
        // We must use regular pointers to avoid circular references.
        return context.get();
      }));
    }
  }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   */
  auto try_steal() noexcept -> task_handle {

    std::size_t multiplier = 1 + m_neigh.size();

    for (auto &&friends : m_neigh) {

      multiplier -= 1;

      LF_ASSERT(multiplier > 0);

      for (std::size_t i = 0; i < k_steal_attempts * multiplier; ++i) {

        std::shuffle(friends.begin(), friends.end(), m_rng);

        for (numa_vars *context : friends) {

          LF_ASSERT(context->m_context);

          auto [err, task] = context->m_context->try_steal();

          switch (err) {
            case lf::err::none:
              LF_LOG("Stole task from {}", (void *)context);
              return task;

            case lf::err::lost:
              // We don't retry here as we don't want to cause contention
              // and we have multiple steal attempts anyway.
            case lf::err::empty:
              continue;

            default:
              LF_ASSERT(false);
          }
        }
      }
    }

    return nullptr;
  }
};

} // namespace impl

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 * Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class busy_pool {

  struct shared_vars {

    explicit shared_vars(std::size_t n) : max_parallel(n), latch_start(n + 1), latch_stop(n + 1) {}

    std::size_t max_parallel;
    std::latch latch_start;
    std::latch latch_stop;
    std::atomic_flag stop;
  };

  xoshiro m_rng{seed, std::random_device{}};

  std::shared_ptr<shared_vars> m_share;
  std::vector<std::shared_ptr<impl::numa_vars>> m_worker;
  std::vector<std::thread> m_threads;

  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {

    LF_LOG("Requesting a stop");
    // Set conditions for workers to stop
    m_share->stop.test_and_set(std::memory_order_release);

    m_share->latch_stop.arrive_and_wait();

    for (auto &worker : m_threads) {
      worker.join();
    }
  }

  static auto work(numa_topology::numa_node<impl::numa_vars> node, std::shared_ptr<shared_vars> vars) {

    LF_ASSERT(!node.neighbors.empty());
    LF_ASSERT(!node.neighbors.front().empty());

    // ------- Initialize my numa variables

    std::shared_ptr my_vars = node.neighbors.front().front();

    worker_context *my_context = worker_init(vars->max_parallel);

    my_vars->init_numa_and_bind(my_context, node);

    vars->latch_start.arrive_and_wait(); // Wait for everyone to have set up their numa_vars.

    // -------

    while (!vars->stop.test(std::memory_order_acquire)) {

      for_each_elem(my_context->try_pop_all(), [](lf::submit_handle submitted) LF_STATIC_CALL noexcept {
        resume(submitted);
      });

      if (task_handle task = my_vars->try_steal()) {
        resume(task);
      }
    };

    vars->latch_stop.arrive_and_wait(); // Wait for everyone to have stopped before destroying the context.

    finalize(my_context);
  }

 public:
  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   * @param strategy The numa strategy for distributing workers.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency(),
                     numa_strategy strategy = numa_strategy::fan)

      : m_share(std::make_shared<shared_vars>(n)) {

    for (std::size_t i = 0; i < n; ++i) {
      m_worker.push_back(std::make_shared<impl::numa_vars>(m_rng));
      m_rng.long_jump();
    }

    LF_ASSERT_NO_ASSUME(!m_share->stop.test(std::memory_order_acquire));

    std::vector nodes = numa_topology{}.distribute(m_worker, strategy);

    // clang-format off

    LF_TRY {
      for (auto &&node : nodes) {
        m_threads.emplace_back(work, std::move(node), m_share);
      }
    } LF_CATCH_ALL {
      clean_up();
      LF_RETHROW;
    }

    // clang-format on

    m_share->latch_start.arrive_and_wait(); // Wait for everyone to have set up their numa_vars.
  }

  ~busy_pool() noexcept { clean_up(); }

  /**
   * @brief Schedule a task for execution.
   */
  void schedule(lf::intruded_list<lf::submit_handle> jobs) {
    std::uniform_int_distribution<std::size_t> dist(0, m_threads.size() - 1);
    m_worker[dist(m_rng)]->submit(jobs);
  }
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
