#ifndef C1BED09D_40CC_4EA1_B687_38A5BCC31907
#define C1BED09D_40CC_4EA1_B687_38A5BCC31907

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <atomic>
#include <bit>
#include <latch>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <thread>

#include "libfork/core.hpp"

#include <libfork/schedule/busy_pool.hpp>

#include "libfork/schedule/ext/event_count.hpp"
#include "libfork/schedule/ext/numa.hpp"
#include "libfork/schedule/ext/random.hpp"

#include "libfork/schedule/impl/contexts.hpp"

/**
 * @file lazy_pool.hpp
 *
 * @brief A work-stealing thread pool where threads sleep when idle.
 */

namespace lf {

namespace impl {

static constexpr std::memory_order acquire = std::memory_order_acquire;
static constexpr std::memory_order acq_rel = std::memory_order_acq_rel;
static constexpr std::memory_order release = std::memory_order_release;

static constexpr std::uint64_t k_thieve = 1;
static constexpr std::uint64_t k_active = k_thieve << 32U;

static constexpr std::uint64_t k_thieve_mask = (k_active - 1);
static constexpr std::uint64_t k_active_mask = ~k_thieve_mask;

/**
 * @brief A collection of heap allocated atomic variables used for tracking the state of the scheduler.
 */
struct lazy_vars {

  alignas(k_cache_line) std::atomic_uint64_t dual_count = 0; ///< The worker + active counters
  alignas(k_cache_line) event_count notifier;                ///< The pools notifier.

  /**
   * Effect:
   *
   * T <- T - 1
   * S <- S
   * A <- A + 1
   *
   * A is now guaranteed to be greater than 0, if we were the last thief we try to wake someone.
   *
   * Then we do the task.
   *
   * Once we are done we perform:
   *
   * T <- T + 1
   * S <- S
   * A <- A - 1
   *
   * This never invalidates the invariant.
   *
   * Overall effect: thief->active, do the work, active->thief.
   */
  template <typename Handle>
    requires std::same_as<Handle, task_handle> || std::same_as<Handle, intruded_list<submit_handle>>
  void thief_round_trip(Handle handle) noexcept {

    auto prev_thieves = dual_count.fetch_add(k_active - k_thieve, acq_rel) & k_thieve_mask;

    if (prev_thieves == 1) {
      LF_LOG("The last thief wakes someone up");
      notifier.notify_one();
    }

    if constexpr (std::same_as<Handle, intruded_list<submit_handle>>) {
      for_each_elem(handle, [](submit_handle submitted) LF_STATIC_CALL noexcept {
        resume(submitted);
      });
    } else {
      resume(handle);
    }

    dual_count.fetch_sub(k_active - k_thieve, acq_rel);
  }
};

struct lazy_group : busy_vars, std::vector<lazy_vars> {
  explicit lazy_group(std::size_t n) : busy_vars(n) {}
};

/**
 * @brief The function that workers run while the pool is alive.
 */
inline auto lazy_work(numa_topology::numa_node<numa_context<lazy_group>> node) noexcept {

  LF_ASSERT(!node.neighbors.empty());
  LF_ASSERT(!node.neighbors.front().empty());

  // ---- Initialization ---- //

  std::shared_ptr my_context = node.neighbors.front().front();

  auto &my_lazy_vars = my_context->shared()[node.numa];

  lf::nullary_function_t notify{[&my_lazy_vars]() {
    my_lazy_vars.notifier.notify_all();
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
   *  If there is an active task there is always: [at least one thief] OR [no sleeping].
   *
   * Let:
   *  T = number of thieves
   *  S = number of sleeping threads
   *  A = number of active threads
   *
   * Invariant: *** if (A > 0) then (T >= 1 OR S == 0) ***
   *
   * Lemma 1: Promoting an S -> T guarantees that the invariant is upheld.
   *
   * Proof 1:
   *  Case S != 0, then T -> T + 1, hence T > 0 hence invariant maintained.
   *  Case S == 0, then invariant is already maintained.
   */

wake_up:
  /**
   * Invariant maintained by Lemma 1 regardless if this is a wake up (S <- S - 1) or join (S <- S).
   */
  my_lazy_vars.dual_count.fetch_add(k_thieve, release);

continue_as_thief:
  /**
   * First we handle the fast path (work to do) before touching the notifier.
   */
  if (auto *submission = my_context->try_pop_all()) {
    my_lazy_vars.thief_round_trip(submission);
    goto continue_as_thief;
  }
  if (auto *stolen = my_context->try_steal()) {
    my_lazy_vars.thief_round_trip(stolen);
    goto continue_as_thief;
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

  auto key = my_lazy_vars.notifier.prepare_wait();

  if (auto *submission = my_context->try_pop_all()) {
    // Check our private **before** `stop`.
    my_lazy_vars.notifier.cancel_wait();
    my_lazy_vars.thief_round_trip(submission);
    goto continue_as_thief;
  }

  if (my_context->shared().stop.test(acquire)) {
    // A stop has been requested, we will honor it under the assumption
    // that the requester has ensured that everyone is done. We cannot check
    // this i.e it is possible a thread that just signaled the master thread
    // is still `active` but act stalled.
    my_lazy_vars.notifier.cancel_wait();
    // We leave a "ghost thief" here e.g. don't bother to reduce the counter,
    // This is fine because no-one can sleep now that the stop flag is set.
    return;
  }

  /**
   * Try:
   *
   * T <- T - 1
   * S <- S + 1
   * A <- A
   *
   * If new T == 0 and A > 0 then wake self immediately i.e:
   *
   * T <- T + 1
   * S <- S - 1
   * A <- A
   *
   * If we return true then we are safe to sleep, otherwise we must stay awake.
   */

  auto prev_dual = my_lazy_vars.dual_count.fetch_sub(k_thieve, acq_rel);

  // We are now registered as a sleeping thread and may have broken the invariant.

  auto prev_thieves = prev_dual & k_thieve_mask;
  auto prev_actives = prev_dual & k_active_mask; // Again only need 0 or non-zero.

  if (prev_thieves == 1 && prev_actives != 0) {
    // Restore the invariant.
    goto wake_up;
  }

  LF_LOG("Goes to sleep");

  // We are safe to sleep.
  my_lazy_vars.notifier.wait(key);
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
  std::shared_ptr<impl::lazy_group> m_share = std::make_shared<impl::lazy_group>(m_num_threads);
  std::vector<std::shared_ptr<impl::numa_context<impl::lazy_group>>> m_worker = {};
  std::vector<std::thread> m_threads = {};

  using strategy = numa_strategy;

 public:
  /**
   * @brief Construct a new lazy_pool object and `n` worker threads.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   * @param strategy The numa strategy for distributing workers.
   */
  explicit lazy_pool(std::size_t n = std::thread::hardware_concurrency(), strategy strategy = strategy::fan)
      : m_num_threads(n) {

    LF_ASSERT_NO_ASSUME(!m_share->stop.test(std::memory_order_acquire));

    for (std::size_t i = 0; i < n; ++i) {
      m_worker.push_back(std::make_shared<impl::numa_context<impl::lazy_group>>(m_rng, m_share));
      m_rng.long_jump();
    }

    std::vector nodes = numa_topology{}.distribute(m_worker, strategy);

    LF_ASSERT(!nodes.empty());

    std::size_t num_numa = 1 + std::ranges::max_element(nodes, {}, [](auto const &node) {
                                 return node.numa;
                               })->numa;

    LF_LOG("Lazy pool has {} numa nodes", num_numa);

    static_cast<std::vector<impl::lazy_vars> &>(*m_share) = std::vector<impl::lazy_vars>(num_numa);

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
  }

  void schedule(lf::intruded_list<lf::submit_handle> jobs) { m_worker[m_dist(m_rng)]->submit(jobs); }

  ~lazy_pool() noexcept {
    LF_LOG("Requesting a stop");

    // Set conditions for workers to stop.
    m_share->stop.test_and_set(std::memory_order_release);

    for (auto &&var : *m_share) {
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
