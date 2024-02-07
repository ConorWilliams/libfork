#ifndef C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC
#define C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC

#include <atomic> // for atomic_flag, ATOMIC_FLAG_INIT, memory_order_acq...
#include <thread> // for thread

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/core/defer.hpp"        // for LF_DEFER
#include "libfork/core/ext/context.hpp"  // for worker_context, nullary_function_t
#include "libfork/core/ext/handles.hpp"  // for submit_handle
#include "libfork/core/ext/resume.hpp"   // for resume
#include "libfork/core/ext/tls.hpp"      // for finalize, worker_init
#include "libfork/core/impl/utility.hpp" // for non_null, immovable

/**
 * @file unit_pool.hpp
 *
 * @brief A scheduler that runs all tasks on a single thread.
 */

namespace lf {

/**
 * @brief A scheduler that runs all tasks on a single thread.
 *
 * This is useful for testing/debugging/benchmarking.
 */
class unit_pool : impl::immovable<unit_pool> {

  static void work(unit_pool *self) {

    worker_context *me = lf::worker_init(lf::nullary_function_t{[]() {}});

    LF_DEFER { lf::finalize(me); };

    self->m_context = me;
    self->m_ready.test_and_set(std::memory_order_release);
    self->m_ready.notify_one();

    while (!self->m_stop.test(std::memory_order_acquire)) {
      if (auto *job = me->try_pop_all()) {
        lf::resume(job);
      }
    }
  }

 public:
  /**
   * @brief Construct a new unit pool.
   */
  unit_pool() : m_thread{work, this} {
    // Wait until worker sets the context.
    m_ready.wait(false, std::memory_order_acquire);
  }

  /**
   * @brief Run a job inline.
   */
  void schedule(submit_handle job) { non_null(m_context)->schedule(job); }

  /**
   * @brief Destroy the unit pool object, waits for the worker to finish.
   */
  ~unit_pool() noexcept {
    m_stop.test_and_set(std::memory_order_release);
    m_thread.join();
  }

 private:
  std::atomic_flag m_stop = ATOMIC_FLAG_INIT;
  std::atomic_flag m_ready = ATOMIC_FLAG_INIT;

  lf::worker_context *m_context = nullptr;

  std::thread m_thread;
};

} // namespace lf

#endif /* C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC */
