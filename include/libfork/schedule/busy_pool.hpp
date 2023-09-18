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

#include "libfork/core/list.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core.hpp"

#include "libfork/schedule/deque.hpp"
#include "libfork/schedule/random.hpp"
#include "libfork/schedule/ring_buffer.hpp"

/**
 * @file busy.hpp
 *
 * @brief A work-stealing thread pool where all the threads spin when idle.
 */

namespace lf {

namespace detail {

// template <typename Steal>
//   requires requires(Steal steal) {
//     { std::invoke(steal) } -> std::convertible_to<std::optional<typename Stack::handle>>;
//   }
// class stack_controller {
//  public:
//   using handle_t = typename Stack::handle;

//   explicit constexpr stack_controller(Steal const &steal) : m_steal{steal} {}

//   auto stack_top() -> handle_t { return m_stacks.peek(); }

//   void stack_pop() {
//     LF_LOG("stack_pop()");

//     LF_ASSERT(!m_stacks.empty());

//     m_stacks.pop();

//     if (!m_stacks.empty()) {
//       return;
//     }

//     LF_LOG("No stack, stealing from other threads");

//     if (std::optional handle = std::invoke(m_steal)) {
//       LF_ASSERT(m_stacks.empty());
//       m_stacks.push(*handle);
//     }

//     LF_LOG("No stacks found, allocating new stacks");
//     alloc_stacks();
//   }

//   void stack_push(handle_t handle) {
//     LF_LOG("Pushing stack to private deque");
//     LF_ASSERT(stack_top()->empty());
//     m_stacks.push(handle);
//   }

//  private:
//   using stack_block = typename Stack::unique_arr_ptr_t;

//   static constexpr std::size_t k_buff = 16;

//   Steal m_steal;
//   ring_buffer<typename Stack::handle, k_buff> m_stacks;
//   std::vector<stack_block> m_stack_storage;

//   void alloc_stacks() {

//     LF_ASSERT(m_stacks.empty());

//     stack_block stacks = Stack::make_unique(k_buff);

//     for (std::size_t i = 0; i < k_buff; ++i) {
//       m_stacks.push(handle_t{stacks.get() + i});
//     }

//     m_stack_storage.push_back(std::move(stacks));
//   }
// };

} // namespace detail

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

  xoshiro m_rng;                           ///< Our personal PRNG.
  std::vector<worker_context *> m_friends; ///< Other contexts in the pool, all non-null.

  template <typename T>
  static constexpr auto null_for = []() LF_STATIC_CONST noexcept -> T * { return nullptr; };

 public:
  worker_context() {
    for (auto *elem : m_friends) {
      LF_ASSERT(elem);
    }
    for (std::size_t i = 0; i < k_buff / 2; ++i) {
      m_buffer.push(new async_stack);
    }
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

  void set_rng(xoshiro const &rng) noexcept { m_rng = rng; }

  void add_friend(worker_context *a_friend) noexcept { m_friends.push_back(impl::non_null(a_friend)); }

  auto max_threads() const noexcept -> std::size_t { return m_friends.size() + 1; }

  auto submit(intrusive_node<frame_block *> *node) noexcept -> void { m_submit.push(impl::non_null(node)); }

  auto resume_submitted() noexcept -> bool {
    return m_submit.consume([](frame_block *submitted) LF_STATIC_CALL noexcept {
      submitted->resume_external<worker_context>();
      //
    });
  }

  auto steal_and_resume() -> bool {
    for (std::size_t i = 0; i < k_steal_attempts; ++i) {

      std::shuffle(m_friends.begin(), m_friends.end(), m_rng);

      for (worker_context *context : m_friends) {
        if (auto task = context->m_tasks.steal()) {
          LF_LOG("Stole task from {}", (void *)context);
          (*task)->resume_stolen();
          LF_ASSERT(m_tasks.empty());
          return true;
        }
      }
    }
    return false;
  }

  auto stack_pop() -> async_stack * {
    if (auto *stack = m_buffer.pop(null_for<async_stack>)) {
      LF_LOG("Using local-buffered stack");
      return stack;
    }
    if (auto *stack = m_stacks.pop(null_for<async_stack>)) {
      LF_LOG("Using public-buffered stack}");
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

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 */
class busy_pool {
 public:
  using context_type = worker_context;

 private:
  xoshiro m_rng{seed, std::random_device{}};

  std::vector<context_type> m_contexts;
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

  static auto work(context_type *my_context, std::atomic_flag const &stop_requested) {

    worker_init(my_context);

    while (!stop_requested.test(std::memory_order_acquire)) {
      my_context->resume_submitted();
      my_context->steal_and_resume();
    };

    worker_finalize(my_context);
  }

 public:
  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {

    for (auto &context : m_contexts) {
      context.set_rng(m_rng);
      m_rng.long_jump();
    }

    for (std::size_t i = 0; i < n; ++i) {
      for (std::size_t j = 0; j < n; ++j) {
        if (i != j) {
          m_contexts[i].add_friend(&m_contexts[j]);
        }
      }
    }

    LF_ASSERT(!m_stop->test(std::memory_order_relaxed));

    LF_TRY {
      for (auto &context : m_contexts) {
        m_workers.emplace_back(work, &context, std::cref(*m_stop));
      }
    }
    LF_CATCH_ALL {
      clean_up();
      LF_RETHROW;
    }
  }

  ~busy_pool() noexcept { clean_up(); }

  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(intrusive_node<frame_block *> *node) noexcept {
    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);
    m_contexts[dist(m_rng)].submit(node);
  }
};

static_assert(scheduler<busy_pool>);

} // namespace ext

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
