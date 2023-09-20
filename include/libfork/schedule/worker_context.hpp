#ifndef C1B42944_8E33_4F6B_BAD6_5FB687F6C737
#define C1B42944_8E33_4F6B_BAD6_5FB687F6C737

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

#include "libfork/core.hpp"
#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/schedule/deque.hpp"
#include "libfork/schedule/random.hpp"
#include "libfork/schedule/ring_buffer.hpp"

/**
 * @file worker_context.hpp
 *
 * @brief A generic `thread_context` suitable for all `libforks` internal multi-threaded schedulers.
 */

namespace lf {

inline namespace ext {

/**
 * @brief The context type for the busy_pools threads.
 *
 * This object does not manage worker_init/worker_finalize as it is intended
 * to be constructed/destructed by the master thread.
 */
class worker_context : impl::immovable<worker_context> {

  static constexpr std::size_t k_buff = 16;
  static constexpr std::size_t k_steal_attempts = 64;

  deque<frame_block *> m_tasks;                ///< Our public task queue, all non-null.
  deque<async_stack *> m_stacks;               ///< Our public stack queue, all non-null.
  intrusive_list<frame_block *> m_submit;      ///< The public submission queue, all non-null.
  ring_buffer<async_stack *, k_buff> m_buffer; ///< Our private stack buffer, all non-null.

  xoshiro m_rng{seed, std::random_device{}}; ///< Our personal PRNG.
  std::vector<worker_context *> m_friends;   ///< Other contexts in the pool, all non-null.

  template <typename T>
  static constexpr auto null_for = []() LF_STATIC_CALL noexcept -> T * {
    return nullptr;
  };

 public:
  worker_context() {
    for (auto *elem : m_friends) {
      LF_ASSERT(elem);
    }
    for (std::size_t i = 0; i < k_buff / 2; ++i) {
      m_buffer.push(new async_stack);
    }
  }

  void set_rng(xoshiro const &rng) noexcept { m_rng = rng; }

  void add_friend(worker_context *a_friend) noexcept { m_friends.push_back(impl::non_null(a_friend)); }

  /**
   * @brief Call `resume_external` on all the submitted tasks, returns `false` if the queue was empty.
   */
  auto try_resume_submitted() noexcept -> bool {
    return m_submit.consume([](frame_block *submitted) LF_STATIC_CALL noexcept {
      submitted->resume_external<worker_context>();
      //
    });
  }

  /**
   * @brief Try to steal a task from one of our friends and call `resume_stolen` on it, returns `false` if we failed.
   */
  auto try_steal_and_resume() -> bool {
    for (std::size_t i = 0; i < k_steal_attempts; ++i) {

      std::shuffle(m_friends.begin(), m_friends.end(), m_rng);

      for (worker_context *context : m_friends) {

        auto [err, task] = context->m_tasks.steal();

        switch (err) {
          case lf::err::none:
            LF_LOG("Stole task from {}", (void *)context);
            task->resume_stolen();
            LF_ASSERT(m_tasks.empty());
            return true;

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
    return false;
  }

  ~worker_context() noexcept {
    //
    LF_ASSERT(m_tasks.empty());

    while (auto *stack = m_buffer.pop(null_for<async_stack>)) {
      delete stack;
    }

    while (auto *stack = m_stacks.pop(null_for<async_stack>)) {
      delete stack;
    }
  }

  // ------------- To satisfy `thread_context` ------------- //

  auto max_threads() const noexcept -> std::size_t { return m_friends.size() + 1; }

  auto submit(intrusive_node<frame_block *> *node) noexcept -> void { m_submit.push(impl::non_null(node)); }

  auto stack_pop() -> async_stack * {

    if (auto *stack = m_buffer.pop(null_for<async_stack>)) {
      LF_LOG("Using local-buffered stack");
      return stack;
    }

    if (auto *stack = m_stacks.pop(null_for<async_stack>)) {
      LF_LOG("Using public-buffered stack");
      return stack;
    }

    std::shuffle(m_friends.begin(), m_friends.end(), m_rng);

    for (worker_context *context : m_friends) {

    retry:
      auto [err, stack] = context->m_stacks.steal();

      switch (err) {
        case lf::err::none:
          LF_LOG("Stole stack from {}", (void *)context);
          return stack;
        case lf::err::lost:
          // We retry (even if it may cause contention) to try and avoid allocating.
          goto retry;
        case lf::err::empty:
          break;
        default:
          LF_ASSERT(false);
      }
    }

    return new async_stack;
  }

  void stack_push(async_stack *stack) {
    m_buffer.push(impl::non_null(stack), [&](async_stack *stack) noexcept {
      LF_LOG("Local stack buffer overflows");
      m_stacks.push(stack);
    });
  }

  auto task_pop() noexcept -> frame_block * { return m_tasks.pop(null_for<frame_block>); }

  void task_push(frame_block *task) { m_tasks.push(impl::non_null(task)); }
};

static_assert(thread_context<worker_context>);

} // namespace ext

} // namespace lf

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */
