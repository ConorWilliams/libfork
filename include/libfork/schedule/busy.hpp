#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>
#include <random>
#include <thread>

#include "libfork/event_count.hpp"
#include "libfork/libfork.hpp"
#include "libfork/macro.hpp"
#include "libfork/queue.hpp"
#include "libfork/random.hpp"
#include "libfork/thread_local.hpp"

/**
 * @file busy.hpp
 *
 * @brief A work-stealing threadpool where all the threads spin when idle.
 */

namespace lf {

namespace detail {

template <simple T, std::size_t N>
  requires(std::has_single_bit(N))
class buffered_queue {
public:
  [[nodiscard]] auto empty() const noexcept -> bool { return m_top == m_bottom; }

  // See what would be popped
  [[nodiscard]] auto peek() -> T & {
    LIBFORK_ASSERT(!empty());
    return load(m_bottom - 1);
  }

  auto push(T const &val) noexcept -> void {
    if (buff_full()) {
      m_queue.push(load(m_top++));
    }
    store(m_bottom++, val);
  }

  auto pop() noexcept -> std::optional<T> {

    if (empty()) {
      return std::nullopt;
    }

    bool was_full = buff_full();

    T val = load(--m_bottom);

    if (was_full) {
      if (auto opt = m_queue.pop()) {
        store(--m_top, *opt);
      }
    }

    return val;
  }

  auto steal() noexcept -> typename queue<T>::steal_t { return m_queue.steal(); }

private:
  [[nodiscard]] auto buff_full() const noexcept -> bool { return m_bottom - m_top == N; }

  auto store(std::size_t index, T const &val) noexcept -> void {
    m_buff[index & mask] = val; // NOLINT
  }

  [[nodiscard]] auto load(std::size_t index) noexcept -> T & {
    return m_buff[(index & mask)]; // NOLINT
  }

  static constexpr std::size_t mask = N - 1;

  std::size_t m_top = 0;
  std::size_t m_bottom = 0;
  std::array<T, N> m_buff;
  queue<T> m_queue;
};

// template <is_virtual_stack Stack, typename Steal>
//   requires requires(Steal steal) {
//     { std::invoke(steal) } -> std::convertible_to<std::optional<typename Stack::handle>>;
//   }
// class stack_controller {
// public:
//   using handle_t = typename Stack::handle;

//   explicit constexpr stack_controller(Steal const &steal) : m_steal{steal} {}

//   auto stack_top() -> handle_t {
//     return m_stacks.peek();
//   }

//   void stack_pop() {
//     LIBFORK_LOG("stack_pop()");

//     LIBFORK_ASSERT(!m_stacks.empty());

//     m_stacks.pop();

//     if (!m_stacks.empty()) {
//       return;
//     }

//     LIBFORK_LOG("No stack, stealing from other threads");

//     if (std::optional handle = std::invoke(m_steal)) {
//       LIBFORK_ASSERT(m_stacks.empty());
//       m_stacks.push(*handle);
//     }

//     LIBFORK_LOG("No stacks found, allocating new stacks");
//     alloc_stacks();
//   }

//   void stack_push(handle_t handle) {
//     LIBFORK_LOG("Pushing stack to private queue");
//     LIBFORK_ASSERT(stack_top()->empty());
//     m_stacks.push(handle);
//   }

// private:
//   using stack_block = typename Stack::unique_arr_ptr_t;

//   static constexpr std::size_t k_buff = 16;

//   Steal m_steal;
//   buffered_queue<typename Stack::handle, k_buff> m_stacks;
//   std::vector<stack_block> m_stack_storage;

//   void alloc_stacks() {

//     LIBFORK_ASSERT(m_stacks.empty());

//     stack_block stacks = Stack::make_unique(k_buff);

//     for (std::size_t i = 0; i < k_buff; ++i) {
//       m_stacks.push(handle_t{stacks.get() + i});
//     }

//     m_stack_storage.push_back(std::move(stacks));
//   }
// };

} // namespace detail

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 */
class busy_pool : detail::immovable {
public:
  /**
   * @brief The context type for the busy_pools threads.
   */
  class context_type : thread_local_ptr<context_type> {
  public:
    using stack_type = virtual_stack<detail::mebibyte>;

    context_type() { alloc_stacks(); }

    static auto context() -> context_type & { return get(); }

    constexpr auto max_threads() const noexcept -> std::size_t { return m_pool->m_workers.size(); }

    auto stack_top() -> stack_type::handle {
      LIBFORK_ASSERT(&context() == this);
      return m_stacks.peek();
    }

    void stack_pop() {
      LIBFORK_ASSERT(&context() == this);
      LIBFORK_ASSERT(!m_stacks.empty());

      LIBFORK_LOG("Pop stack");

      m_stacks.pop();

      if (!m_stacks.empty()) {
        return;
      }

      LIBFORK_LOG("No stack, stealing from other threads");

      auto n = max_threads();

      std::uniform_int_distribution<std::size_t> dist(0, n - 1);

      for (std::size_t attempts = 0; attempts < 2 * n;) {

        auto steal_at = dist(m_rng);

        if (steal_at == m_id) {
          continue;
        }

        if (auto handle = m_pool->m_contexts[steal_at].m_stacks.steal()) {
          LIBFORK_LOG("Stole stack from thread {}", steal_at);
          LIBFORK_ASSERT(m_stacks.empty());
          m_stacks.push(*handle);
          return;
        }

        ++attempts;
      }

      LIBFORK_LOG("No stacks found, allocating new stacks");
      alloc_stacks();
    }

    void stack_push(stack_type::handle handle) {
      LIBFORK_ASSERT(&context() == this);
      LIBFORK_ASSERT(stack_top()->empty());

      LIBFORK_LOG("Pushing stack to private queue");

      m_stacks.push(handle);
    }

    auto task_steal() -> typename queue<task_handle>::steal_t {
      return m_tasks.steal();
    }

    auto task_pop() -> std::optional<task_handle> {
      LIBFORK_ASSERT(&context() == this);
      return m_tasks.pop();
    }

    void task_push(task_handle task) {
      LIBFORK_ASSERT(&context() == this);
      m_tasks.push(task);
    }

  private:
    static constexpr std::size_t block_size = 16;

    using stack_block = typename stack_type::unique_arr_ptr_t;
    using stack_handle = typename stack_type::handle;

    friend class busy_pool;

    busy_pool *m_pool = nullptr; ///< To the pool this context belongs to.
    std::size_t m_id = 0;        ///< Index in the pool.
    xoshiro m_rng;               ///< Our personal PRNG.

    detail::buffered_queue<stack_handle, block_size> m_stacks; ///< Our (stealable) stack queue.
    queue<task_handle> m_tasks;                                ///< Our (stealable) task queue.

    std::vector<stack_block> m_stack_storage; ///< Controls ownership of the stacks.

    // Alloc k new stacks and insert them into our private queue.
    void alloc_stacks() {

      LIBFORK_ASSERT(m_stacks.empty());

      stack_block stacks = stack_type::make_unique(block_size);

      for (std::size_t i = 0; i < block_size; ++i) {
        m_stacks.push(stack_handle{stacks.get() + i});
      }

      m_stack_storage.push_back(std::move(stacks));
    }
  };

  static_assert(thread_context<context_type>);

  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {
    // Initialize the random number generator.
    xoshiro rng(std::random_device{});

    std::size_t count = 0;

    for (auto &ctx : m_contexts) {
      ctx.m_pool = this;
      ctx.m_rng = rng;
      rng.long_jump();
      ctx.m_id = count++;
    }

#if LIBFORK_COMPILER_EXCEPTIONS
    try {
#endif
      for (std::size_t i = 0; i < n; ++i) {
        m_workers.emplace_back([this, i]() {
          // Get a reference to the threads context.
          context_type &my_context = m_contexts[i];

          // Set the thread local context.
          context_type::set(my_context);

          std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

          while (!m_stop_requested.test(std::memory_order_acquire)) {

            for (int attempt = 0; attempt < k_steal_attempts; ++attempt) {

              std::size_t steal_at = dist(my_context.m_rng);

              if (steal_at == i) {
                if (auto root = m_submit.steal()) {
                  LIBFORK_LOG("resuming root task");
                  root->resume();
                }
              } else if (auto work = m_contexts[steal_at].task_steal()) {
                attempt = 0;
                LIBFORK_LOG("Stole work from {}", steal_at);
                work->resume();
                LIBFORK_LOG("worker resumes thieving");
                LIBFORK_ASSUME(my_context.m_tasks.empty());
              }
            }
          };

          LIBFORK_LOG("Worker {} stopping", i);
        });
      }
#if LIBFORK_COMPILER_EXCEPTIONS
    } catch (...) {
      // Need to stop the threads
      clean_up();
      throw;
    }
#endif
  }

  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(stdx::coroutine_handle<> root) noexcept {
    m_submit.push(root);
  }

  ~busy_pool() noexcept { clean_up(); }

private:
  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {

    LIBFORK_LOG("Request stop");

    LIBFORK_ASSERT(m_submit.empty());

    // Set conditions for workers to stop
    m_stop_requested.test_and_set(std::memory_order_release);

    // Join workers
    for (auto &worker : m_workers) {
      LIBFORK_ASSUME(worker.joinable());
      worker.join();
    }
  }

  static constexpr int k_steal_attempts = 1024;

  queue<stdx::coroutine_handle<>> m_submit;

  std::atomic_flag m_stop_requested = ATOMIC_FLAG_INIT;

  std::vector<context_type> m_contexts;
  std::vector<std::thread> m_workers; // After m_context so threads are destroyed before the queues.
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
