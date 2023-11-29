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
#include <cstddef>
#include <libfork/core/context.hpp>
#include <memory>
#include <random>
#include <vector>

#include "libfork/core.hpp"

#include "libfork/schedule/ext/numa.hpp"
#include "libfork/schedule/ext/random.hpp"

/**
 * @file contexts.hpp
 *
 * @brief A collection of `thread_context` implementations for different purposes.
 */

namespace lf::impl {

// --------------------------------------------------------------------- //

/**
 * @brief A generic `thread_context` suitable for all `libfork`s multi-threaded schedulers.
 *
 * This object does not manage worker_init/worker_finalize as it is intended
 * to be constructed/destructed by the master thread.
 */
class numa_worker_context : immovable<numa_worker_context> {

  static constexpr std::size_t k_buff = 16;
  static constexpr std::size_t k_steal_attempts = 32; ///< Attempts per target.

  xoshiro m_rng;                                      ///< Our personal PRNG.
  std::size_t m_max_threads;                          ///< The total max parallelism available.
  worker_context m_context;                           ///< Our context.
  std::vector<std::vector<worker_context *>> m_neigh; ///< Our view of the NUMA topology.

  // template <typename T>
  // static constexpr auto null_for = []() LF_STATIC_CALL noexcept -> T * {
  //   return nullptr;
  // };

 public:
  /**
   * @brief Construct a new numa worker context object.
   *
   * @param n The maximum parallelism.
   * @param rng This worker private pseudo random number generator.
   */
  numa_worker_context(std::size_t n, xoshiro const &rng) : m_rng(rng), m_max_threads(n) {
    for (std::size_t i = 0; i < k_buff / 2; ++i) {
      m_buffer.push(new fibre_stack);
    }
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
  void init_numa_and_bind(numa_topology::numa_node<CRTP> const &topo) {

    LF_ASSERT(topo.neighbors.front().front().get() == this);
    topo.bind();

    // Skip the first one as it is just us.
    for (std::size_t i = 1; i < topo.neighbors.size(); ++i) {
      m_neigh.push_back(map(topo.neighbors[i], [](std::shared_ptr<CRTP> const &context) {
        // We must use regular pointers to avoid circular references.
        return context.get();
      }));
    }
  }

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   */
  auto try_get_submitted() noexcept -> intruded_t * { return m_submit.try_pop_all(); }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   */
  auto try_steal() noexcept -> task_t * {

    std::size_t multiplier = 1 + m_neigh.size();

    for (auto &&friends : m_neigh) {

      multiplier -= 1;

      LF_ASSERT(multiplier > 0);

      for (std::size_t i = 0; i < k_steal_attempts * multiplier; ++i) {

        std::shuffle(friends.begin(), friends.end(), m_rng);

        for (CRTP *context : friends) {

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
    }

    return nullptr;
  }

  ~numa_worker_context() noexcept {
    //
    LF_ASSERT_NO_ASSUME(m_tasks.empty());

    while (auto *stack = m_buffer.pop(null_for<fibre_stack>)) {
      delete stack;
    }

    while (auto *stack = m_stacks.pop(null_for<fibre_stack>)) {
      delete stack;
    }
  }

  // ------------- To satisfy `thread_context` ------------- //

  /**
   * @brief Get the maximum parallelism.
   */
  auto max_threads() const noexcept -> std::size_t { return m_max_threads; }

  /**
   * @brief Submit a node for execution onto this workers private queue.
   */
  auto submit(intruded_t *node) noexcept -> void { m_submit.push(non_null(node)); }

  /**
   * @brief Get a new empty ``fiber_stack``.
   *
   * This will attempt to steal one before allocating.
   */
  auto stack_pop() -> fibre_stack * {

    if (auto *stack = m_buffer.pop(null_for<fibre_stack>)) {
      LF_LOG("stack_pop() using local-buffered stack");
      return stack;
    }

    if (auto *stack = m_stacks.pop(null_for<fibre_stack>)) {
      LF_LOG("stack_pop() using public-buffered stack");
      return stack;
    }

    for (auto &&friends : m_neigh) {

      std::shuffle(friends.begin(), friends.end(), m_rng);

      for (CRTP *context : friends) {

      retry:
        auto [err, stack] = context->m_stacks.steal();

        switch (err) {
          case lf::err::none:
            LF_LOG("stack_pop() stole from {}", (void *)context);
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
    }

    LF_LOG("stack_pop() allocating");

    return new fibre_stack;
  }

  /**
   * @brief Cache an empty fiber stack.
   */
  void stack_push(fibre_stack *stack) {
    m_buffer.push(non_null(stack), [&](fibre_stack *extra_stack) noexcept {
      LF_LOG("Local stack buffer overflows");
      m_stacks.push(extra_stack);
    });
  }

  /**
   * @brief Pop a task from the public task queue.
   */
  LF_FORCEINLINE auto task_pop() noexcept -> task_t * { return m_tasks.pop(null_for<task_t>); }

  /**
   * @brief Add a task to the public task queue.
   */
  LF_FORCEINLINE void task_push(task_t *task) { m_tasks.push(non_null(task)); }
};

namespace detail::static_test {

struct test_context : numa_worker_context<test_context> {};

static_assert(thread_context<test_context>);
static_assert(!single_thread_context<test_context>);

} // namespace detail::static_test

} // namespace lf::impl

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */
