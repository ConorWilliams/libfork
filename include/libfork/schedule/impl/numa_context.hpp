#ifndef C1B42944_8E33_4F6B_BAD6_5FB687F6C737
#define C1B42944_8E33_4F6B_BAD6_5FB687F6C737

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm> // for shuffle
#include <cstddef>   // for size_t
#include <memory>    // for shared_ptr
#include <random>    // for discrete_distribution
#include <utility>   // for exchange, move
#include <vector>    // for vector

#include "libfork/core/ext/context.hpp"    // for worker_context, nullary_f...
#include "libfork/core/ext/deque.hpp"      // for err
#include "libfork/core/ext/handles.hpp"    // for submit_handle, task_handle
#include "libfork/core/ext/list.hpp"       // for intruded_list
#include "libfork/core/ext/tls.hpp"        // for finalize, worker_init
#include "libfork/core/impl/utility.hpp"   // for non_null, map
#include "libfork/core/macro.hpp"          // for LF_ASSERT, LF_LOG, LF_CAT...
#include "libfork/schedule/ext/numa.hpp"   // for numa_topology
#include "libfork/schedule/ext/random.hpp" // for xoshiro

/**
 * @file numa_context.hpp
 *
 * @brief An augmentation of the `worker_context` which tracks the topology of the numa nodes.
 */

// --------------------------------------------------------------------- //
namespace lf::impl {

/**
 * @brief Manages an `lf::worker_context` and exposes numa aware stealing.
 */
template <typename Shared>
struct numa_context {
 private:
  static constexpr std::size_t k_min_steal_attempts = 1024;
  static constexpr std::size_t k_steal_attempts_per_target = 32;

  xoshiro m_rng;                                  ///< Thread-local RNG.
  std::shared_ptr<Shared> m_shared;               ///< Shared variables between all numa_contexts.
  worker_context *m_context = nullptr;            ///< The worker context we are associated with.
  std::discrete_distribution<std::size_t> m_dist; ///< The distribution for stealing.
  std::vector<numa_context *> m_close;            ///< First order neighbors.
  std::vector<numa_context *> m_neigh;            ///< Our neighbors (excluding ourselves).

 public:
  /**
   * @brief Construct a new numa context object.
   */
  numa_context(xoshiro const &rng, std::shared_ptr<Shared> shared)
      : m_rng(rng),
        m_shared{std::move(non_null(shared))} {}

  /**
   * @brief Get access to the shared variables.
   */
  [[nodiscard]] auto shared() const noexcept -> Shared & { return *non_null(m_shared); }

  /**
   * @brief An alias for `numa_topology::numa_node<numa_context<Shared>>`.
   */
  using numa_node = numa_topology::numa_node<numa_context>;

  /**
   * @brief Initialize the underlying worker context and bind calling thread a hardware core.
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
  void init_worker_and_bind(nullary_function_t notify, numa_node const &topo) {

    LF_ASSERT(!topo.neighbors.empty());
    LF_ASSERT(!topo.neighbors.front().empty());
    LF_ASSERT(topo.neighbors.front().front().get() == this);

    LF_ASSERT(m_neigh.empty()); // Should only be called once.

    topo.bind();

    m_context = worker_init(std::move(notify));

    std::vector<double> weights;

    // clang-format off

    LF_TRY {

      if (topo.neighbors.size() > 1){
        m_close = impl::map(topo.neighbors[1], [](auto const & neigh) {
          return neigh.get();
        });
      }

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

    } LF_CATCH_ALL {
      m_close.clear();
      m_neigh.clear();
      LF_RETHROW;
    }
  }

  /**
   * @brief Call `lf::finalize` on the underlying worker context.
   */
  void finalize_worker() { finalize(std::exchange(m_context, nullptr)); }

  /**
   * @brief Fetch the `lf::context` a thread has associated with this object.
   */
  auto get_underlying() noexcept -> worker_context * { return m_context; }

  /**
   * @brief schedule a job to the owned worker context.
   */
  void schedule(submit_handle job) { non_null(m_context)->schedule(job); }

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   *
   * If there are no submitted tasks, then returned pointer will be null.
   */
  [[nodiscard]] auto try_pop_all() noexcept -> submit_handle {
    return non_null(m_context)->try_pop_all();
  }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   */
  [[nodiscard]] auto try_steal() noexcept -> task_handle {

    if (m_neigh.empty()){
      return nullptr;
    }

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

    #define LF_RETURN_OR_CONTINUE(expr) \
      auto * context = expr;\
      LF_ASSERT(context); \
      LF_ASSERT(context->m_context);\
      auto [err, task] = context->m_context->try_steal();\
\
      switch (err) {\
        case lf::err::none:\
          LF_LOG("Stole task from {}", (void *)context);\
          return task;\
\
        case lf::err::lost:\
          /* We don't retry here as we don't want to cause contention */ \
          /* and we have multiple steal attempts anyway */ \
        case lf::err::empty:\
          continue;\
\
        default:\
          LF_ASSERT(false && "Unreachable");\
      }


    std::ranges::shuffle(m_close, m_rng);

    // Check all of the closest numa domain.
    for (auto * neigh : m_close) {
      LF_RETURN_OR_CONTINUE(neigh);
    }

    std::size_t attempts = k_min_steal_attempts + k_steal_attempts_per_target * m_neigh.size();

    // Then work probabilistically.
    for (std::size_t i = 0; i < attempts; ++i) {
       LF_RETURN_OR_CONTINUE(m_neigh[m_dist(m_rng)]);
    }

#undef LF_RETURN_OR_CONTINUE

#endif // LF_DOXYGEN_SHOULD_SKIP_THIS

    return nullptr;
  }
};

} // namespace lf::impl

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */
