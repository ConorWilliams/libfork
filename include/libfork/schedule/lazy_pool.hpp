#ifndef C1BED09D_40CC_4EA1_B687_38A5BCC31907
#define C1BED09D_40CC_4EA1_B687_38A5BCC31907

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>  // for __max_element_fn, max_element
#include <atomic>     // for atomic_flag, memory_order, memory_orde...
#include <concepts>   // for same_as
#include <cstddef>    // for size_t
#include <functional> // for less
#include <latch>      // for latch
#include <memory>     // for shared_ptr, __shared_ptr_access, make_...
#include <random>     // for random_device, uniform_int_distribution
#include <span>       // for span
#include <thread>     // for thread
#include <utility>    // for move
#include <vector>     // for vector

#include "libfork/core/defer.hpp"                 // for LF_DEFER
#include "libfork/core/ext/context.hpp"           // for worker_context, nullary_function_t
#include "libfork/core/ext/handles.hpp"           // for submit_handle, task_handle
#include "libfork/core/ext/resume.hpp"            // for resume
#include "libfork/core/impl/utility.hpp"          // for k_cache_line
#include "libfork/core/macro.hpp"                 // for LF_ASSERT, LF_LOG, LF_ASSERT_NO_ASSUME
#include "libfork/core/scheduler.hpp"             // for scheduler
#include "libfork/schedule/busy_pool.hpp"         // for busy_vars
#include "libfork/schedule/ext/event_count.hpp"   // for event_count
#include "libfork/schedule/ext/numa.hpp"          // for numa_strategy, numa_topology
#include "libfork/schedule/ext/random.hpp"        // for xoshiro, seed
#include "libfork/schedule/impl/numa_context.hpp" // for numa_context

/**
 * @file lazy_pool.hpp
 *
 * @brief A work-stealing thread pool where threads sleep when idle.
 */

namespace lf {

namespace impl {

static constexpr std::memory_order acquire = std::memory_order_acquire; ///< Alias
static constexpr std::memory_order acq_rel = std::memory_order_acq_rel; ///< Alias
static constexpr std::memory_order release = std::memory_order_release; ///< Alias

/**
 * @brief A collection of heap allocated atomic variables used for tracking the state of the scheduler.
 */
struct lazy_vars : busy_vars {

  using busy_vars::busy_vars;

  /**
   * @brief Counters and notifiers for each numa locality.
   */
  struct fat_counters {
    alignas(k_cache_line) std::atomic_uint64_t thief = 0; ///< Number of thieving workers.
    alignas(k_cache_line) event_count notifier;           ///< Notifier for this numa pool.
  };

  alignas(k_cache_line) std::atomic_uint64_t active = 0; ///< Total number of actives.
  alignas(k_cache_line) std::vector<fat_counters> numa;  ///< Counters for each numa locality.

  // Invariant: *** if (A > 0) then (T >= 1 OR S == 0) ***

  /**
   * Called by a thief with work, effect: thief->active, do work, active->sleep.
   */
  template <typename Handle>
    requires std::same_as<Handle, task_handle> || std::same_as<Handle, submit_handle>
  void thief_work_sleep(Handle handle, std::size_t tid) noexcept {

    // Invariant: *** if (A > 0) then (Ti >= 1 OR Si == 0) for all i***

    // First we transition from thief -> sleep:
    //
    // Ti <- Ti - 1
    // Si <- Si + 1
    //
    // Invariant in numa j != i is uneffected.
    //
    // In numa i we guarantee that Ti >= 1 by waking someone else if we are the last thief as Si != 0.

    if (numa[tid].thief.fetch_sub(1, acq_rel) == 1) {
      numa[tid].notifier.notify_one();
    }

    // Then we transition from sleep -> active
    //
    // A <- A + 1
    // Si <- Si - 1

    // If we are the first active then we need to maintain the invariant across all numa domains.

    if (active.fetch_add(1, acq_rel) == 0) {
      for (auto &&domain : numa) {
        if (domain.thief.load(acquire) == 0) {
          domain.notifier.notify_one();
        }
      }
    }

    resume(handle);

    // Finally A <- A - 1 does not invalidate the invariant in any domain.
    active.fetch_sub(1, release);
  }
};

/**
 * @brief The function that workers run while the pool is alive (worker event-loop)
 */
inline auto lazy_work(numa_topology::numa_node<numa_context<lazy_vars>> node) noexcept {

  LF_ASSERT(!node.neighbors.empty());
  LF_ASSERT(!node.neighbors.front().empty());

  // ---- Initialization ---- //

  std::shared_ptr my_context = node.neighbors.front().front();

  LF_ASSERT(my_context && !my_context->shared().numa.empty());

  std::size_t numa_tid = node.numa;

  auto &my_numa_vars = my_context->shared().numa[numa_tid]; // node.numa

  lf::nullary_function_t notify{[&my_numa_vars]() {
    my_numa_vars.notifier.notify_all();
  }};

  my_context->init_worker_and_bind(std::move(notify), node);

  // Wait for everyone to have set up their numa_vars. If this throws an exception then
  // program terminates due to the noexcept marker.
  my_context->shared().latch_start.arrive_and_wait();

  LF_DEFER {
    // Wait for everyone to have stopped before destroying the context (which others could be observing).
    my_context->shared().stop.test_and_set(std::memory_order_release);
    my_context->shared().latch_stop.arrive_and_wait();
    my_context->finalize_worker();
  };

  // ----------------------------------- //

  /**
   * Invariant we want to uphold:
   *
   *  If there is an active task there is always: [at least one thief] OR [no sleeping] in each numa.
   *
   * Let:
   *  Ti = number of thieves in numa i
   *  Si = number of sleeping threads in numa i
   *  A = number of active threads across all numa domains
   *
   * Invariant: *** if (A > 0) then (Ti >= 1 OR Si == 0) for all i***
   */

  /**
   * Lemma 1: Promoting an Si -> Ti guarantees that the invariant is upheld.
   *
   * Proof 1:
   *  Ti -> Ti + 1, hence Ti > 0, hence invariant maintained in numa i.
   *  In numa j != i invariant is uneffected.
   *
   */

wake_up:
  /**
   * Invariant maintained by Lemma 1.
   */
  my_numa_vars.thief.fetch_add(1, release);

  /**
   * First we handle the fast path (work to do) before touching the notifier.
   */
  if (auto *submission = my_context->try_pop_all()) {
    my_context->shared().thief_work_sleep(submission, numa_tid);
    goto wake_up;
  }
  if (auto *stolen = my_context->try_steal()) {
    my_context->shared().thief_work_sleep(stolen, numa_tid);
    goto wake_up;
  }

  /**
   * Now we are going to try and sleep if the conditions are correct.
   *
   * Event count pattern:
   *
   *    key <- prepare_wait()
   *
   *    Check condition for sleep:
   *      - We have no private work.
   *      - We are not the watch dog.
   *      - The scheduler has not stopped.
   *
   *    Commit/cancel wait on key.
   */

  auto key = my_numa_vars.notifier.prepare_wait();

  if (auto *submission = my_context->try_pop_all()) {
    // Check our private **before** `stop`.
    my_numa_vars.notifier.cancel_wait();
    my_context->shared().thief_work_sleep(submission, numa_tid);
    goto wake_up;
  }

  if (my_context->shared().stop.test(acquire)) {
    // A stop has been requested, we will honor it under the assumption
    // that the requester has ensured that everyone is done. We cannot check
    // this i.e it is possible a thread that just signaled the master thread
    // is still `active` but act stalled.
    my_numa_vars.notifier.cancel_wait();
    my_numa_vars.notifier.notify_all();
    my_numa_vars.thief.fetch_sub(1, release);
    return;
  }

  /**
   * Try:
   *
   * Ti <- Ti - 1
   * Si <- Si + 1
   *
   * If new Ti == 0 then we check A, if A > 0 then wake self immediately i.e:
   *
   * Ti <- Ti + 1
   * Si <- Si - 1
   *
   * This maintains invariant in numa.
   */

  if (my_numa_vars.thief.fetch_sub(1, acq_rel) == 1) {

    // If we are the last thief then invariant may be broken if A > 0 as S > 0 (because we are asleep).

    if (my_context->shared().active.load(acquire) > 0) {
      // Restore the invariant if A > 0 by immediately waking self.
      my_numa_vars.notifier.cancel_wait();
      goto wake_up;
    }
  }

  LF_LOG("Goes to sleep");

  // We are safe to sleep.
  my_numa_vars.notifier.wait(key);
  // Note, this could be a spurious wakeup, that doesn't matter because we will just loop around.
  goto wake_up;
}

} // namespace impl

/**
 * @brief A scheduler based on a [An Efficient Work-Stealing Scheduler for Task Dependency
 * Graph](https://doi.org/10.1109/icpads51040.2020.00018)
 *
 * This pool sleeps workers which cannot find any work, as such it should be the default choice for most
 * use cases. Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class lazy_pool {

  std::size_t m_num_threads;
  std::uniform_int_distribution<std::size_t> m_dist{0, m_num_threads - 1};
  xoshiro m_rng{seed, std::random_device{}};
  std::shared_ptr<impl::lazy_vars> m_share = std::make_shared<impl::lazy_vars>(m_num_threads);
  std::vector<std::shared_ptr<impl::numa_context<impl::lazy_vars>>> m_worker = {};
  std::vector<std::thread> m_threads = {};
  std::vector<worker_context *> m_contexts = {};

 public:
  /**
   * @brief Construct a new lazy_pool object and `n` worker threads.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   * @param strategy The numa strategy for distributing workers.
   */
  explicit lazy_pool(std::size_t n = std::thread::hardware_concurrency(),
                     numa_strategy strategy = numa_strategy::fan)
      : m_num_threads(n) {

    LF_ASSERT_NO_ASSUME(m_share && !m_share->stop.test(std::memory_order_acquire));

    for (std::size_t i = 0; i < n; ++i) {
      m_worker.push_back(std::make_shared<impl::numa_context<impl::lazy_vars>>(m_rng, m_share));
      m_rng.long_jump();
    }

    std::vector nodes = numa_topology{}.distribute(m_worker, strategy);

    LF_ASSERT(!nodes.empty());

    std::size_t num_numa = 1 + std::ranges::max_element(nodes, {}, [](auto const &node) {
                                 return node.numa;
                               })->numa;

    LF_LOG("Lazy pool has {} numa nodes", num_numa);

    m_share->numa = std::vector<impl::lazy_vars::fat_counters>(num_numa);

    [&]() noexcept {
      // All workers must be created, if we fail to create them all then we must terminate else
      // the workers will hang on the latch.
      for (auto &&node : nodes) {
        m_threads.emplace_back(impl::lazy_work, std::move(node));
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
   * @brief Schedule a job on a random worker.
   */
  void schedule(submit_handle job) { m_worker[m_dist(m_rng)]->schedule(job); }

  /**
   * @brief Get a view of the worker's contexts.
   */
  auto contexts() noexcept -> std::span<worker_context *> { return m_contexts; }

  ~lazy_pool() noexcept {
    LF_LOG("Requesting a stop");

    // Set conditions for workers to stop.
    m_share->stop.test_and_set(std::memory_order_release);

    for (auto &&var : m_share->numa) {
      var.notifier.notify_all();
    }

    for (auto &worker : m_threads) {
      worker.join();
    }
  }
};

static_assert(scheduler<lazy_pool>);

} // namespace lf

#endif /* C1BED09D_40CC_4EA1_B687_38A5BCC31907 */
