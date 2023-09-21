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
#include <vector>

#include "libfork/core.hpp"
#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/schedule/deque.hpp"
#include "libfork/schedule/random.hpp"
#include "libfork/schedule/ring_buffer.hpp"

/**
 * @file contexts.hpp
 *
 * @brief A collection of `thread_context` implementations for different purposes.
 */

namespace lf::impl {

template <typename CRTP>
struct immediate_base {

  static void submit(intruded_h<CRTP> *ptr) { resume(unwrap(non_null(ptr))); }

  static void stack_push(async_stack *stack) {
    LF_LOG("stack_push");
    LF_ASSERT(stack);
    delete stack;
  }

  static auto stack_pop() -> async_stack * {
    LF_LOG("stack_pop");
    return new async_stack;
  }
};

// --------------------------------------------------------------------- //

/**
 * @brief The context type for the scheduler.
 */
class immediate_context : public immediate_base<immediate_context> {
 public:
  /**
   * @brief Returns `1` as this runs all tasks inline.
   *
   * \rst
   *
   * .. note::
   *
   *    As this is a static constexpr function the promise will transform all `fork -> call`.
   *
   * \endrst
   */
  static constexpr auto max_threads() noexcept -> std::size_t { return 1; }

  /**
   * @brief This should never be called due to the above.
   */
  auto task_pop() -> task_h<immediate_context> * {
    LF_ASSERT("false");
    return nullptr;
  }

  /**
   * @brief This should never be called due to the above.
   */
  void task_push(task_h<immediate_context> *) { LF_ASSERT("false"); }
};

static_assert(single_thread_context<immediate_context>);

// --------------------------------------------------------------------- //

/**
 * @brief An internal context type for testing purposes.
 *
 * This is essentially an immediate context with a task queue.
 */
class test_immediate_context : public immediate_base<test_immediate_context> {
 public:
  test_immediate_context() { m_tasks.reserve(1024); }

  // Deliberately not constexpr such that the promise will not transform all `fork -> call`.
  static auto max_threads() noexcept -> std::size_t { return 1; }

  /**
   * @brief Pops a task from the task queue.
   */
  auto task_pop() -> task_h<test_immediate_context> * {

    if (m_tasks.empty()) {
      return nullptr;
    }

    auto *last = m_tasks.back();
    m_tasks.pop_back();
    return last;
  }

  /**
   * @brief Pushes a task to the task queue.
   */
  void task_push(task_h<test_immediate_context> *task) { m_tasks.push_back(non_null(task)); }

 private:
  std::vector<task_h<test_immediate_context> *> m_tasks; // All non-null.
};

static_assert(thread_context<test_immediate_context>);
static_assert(!single_thread_context<test_immediate_context>);

// --------------------------------------------------------------------- //

/**
 * @brief A generic `thread_context` suitable for all `libforks` multi-threaded schedulers.
 *
 * This object does not manage worker_init/worker_finalize as it is intended
 * to be constructed/destructed by the master thread.
 */
class worker_context : immovable<worker_context> {
  static constexpr std::size_t k_buff = 16;
  static constexpr std::size_t k_steal_attempts = 64;

  using task_t = task_h<worker_context>;
  using submit_t = submit_h<worker_context>;

  deque<task_t *> m_tasks;                     ///< Our public task queue, all non-null.
  deque<async_stack *> m_stacks;               ///< Our public stack queue, all non-null.
  intrusive_list<submit_t *> m_submit;         ///< The public submission queue, all non-null.
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

  void add_friend(worker_context *a_friend) noexcept { m_friends.push_back(non_null(a_friend)); }

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   */
  auto try_get_submitted() noexcept -> intruded_h<worker_context> * { return m_submit.try_pop_all(); }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   *
   * Call `resume_stolen` on it if we succeeded.
   */
  auto try_steal() -> task_t * {
    for (std::size_t i = 0; i < k_steal_attempts; ++i) {

      std::shuffle(m_friends.begin(), m_friends.end(), m_rng);

      for (worker_context *context : m_friends) {

        auto [err, task] = context->m_tasks.steal();

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
    return nullptr;
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

  auto submit(intruded_h<worker_context> *node) noexcept -> void { m_submit.push(non_null(node)); }

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
    m_buffer.push(non_null(stack), [&](async_stack *extra_stack) noexcept {
      LF_LOG("Local stack buffer overflows");
      m_stacks.push(extra_stack);
    });
  }

  auto task_pop() noexcept -> task_t * { return m_tasks.pop(null_for<task_t>); }

  void task_push(task_t *task) { m_tasks.push(non_null(task)); }
};

static_assert(thread_context<worker_context>);
static_assert(!single_thread_context<worker_context>);

} // namespace lf::impl

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */
