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
#include <memory>
#include <random>
#include <vector>

#include "libfork/core.hpp"

#include "libfork/schedule/ext/numa.hpp"
#include "libfork/schedule/ext/random.hpp"

/**
 * @file contexts.hpp
 *
 * @brief An augmentation of the `worker_context` which tracks the topology of the numa nodes.
 */

// --------------------------------------------------------------------- //
namespace lf::impl {

template <typename Shared>
struct numa_context {
 private:
  static constexpr std::size_t k_steal_attempts = 32; ///< Attempts per target.

  std::size_t m_max_parallel;                     ///< The maximum number of parallel tasks.
  xoshiro m_rng;                                  ///< Thread-local RNG.
  std::shared_ptr<Shared> m_shared;               ///< Shared variables between all numa_contexts.
  worker_context *m_context = nullptr;            ///< The worker context we are associated with.
  std::discrete_distribution<std::size_t> m_dist; ///< The distribution for stealing.
  std::vector<numa_context *> m_neigh;            ///< Our neighbors (excluding ourselves).

 public:
  numa_context(std::size_t max_parallel, xoshiro const &rng, std::shared_ptr<Shared> shared)
      : m_max_parallel(max_parallel),
        m_rng(rng),
        m_shared{std::move(non_null(shared))} {}

  auto shared() const noexcept -> Shared & { return *non_null(m_shared); }

  auto worker_context() const noexcept -> worker_context & { return *non_null(m_context); }

  /**
   * @brief Initialize the context and worker with the given topology and bind it to a hardware core.
   *
   * This is separate from construction as the master thread will need to construct
   * the contexts before they can form a reference to them, this must be called by the
   * worker thread which should eventually call `finalize`.
   *
   * The context will store __raw__ pointers to the other contexts in the topology, this is
   * to ensure no circular references are formed.
   *
   * The lifetime of the `context` and `topo` neighbors must outlive all use of this object (excluding
   * destruction).
   */
  void init_worker_and_bind(numa_topology::numa_node<numa_context> const &topo) {

    LF_ASSERT(!topo.neighbors.empty());
    LF_ASSERT(!topo.neighbors.front().empty());
    LF_ASSERT(topo.neighbors.front().front().get() == this);

    LF_ASSERT(m_neigh.empty()); // Should only be called once.

    topo.bind();

    std::vector<double> weights;

    // Skip the first one as it is just us.
    for (std::size_t i = 1; i < topo.neighbors.size(); ++i) {

      double n = topo.neighbors[i].size();
      double w = 1 / (n * i * i);

      for (auto &&context : topo.neighbors[i]) {
        weights.push_back(w);
        m_neigh.push_back(context.get());
      }
    }

    m_dist = std::discrete_distribution<std::size_t>{weights.begin(), weights.end()};

    // Last thing in-case method throws.
    m_context = worker_init(m_max_parallel);
  }

  void finalize_worker() { finalize(std::exchange(m_context, nullptr)); }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   */
  auto try_steal() noexcept -> task_handle {

    for (std::size_t i = 0; i < k_steal_attempts * m_neigh.size(); ++i) {

      numa_context *context = m_neigh[m_dist(m_rng)];

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

    return nullptr;
  }
};

} // namespace lf::impl

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */
