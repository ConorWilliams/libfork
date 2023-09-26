#ifndef D8877F11_1F66_4AD0_B949_C0DFF390C2DB
#define D8877F11_1F66_4AD0_B949_C0DFF390C2DB

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cerrno>
#include <climits>
#include <cstddef>
#include <limits>
#include <memory>
#include <set>
#include <vector>

#include "libfork/core.hpp"
#include "libfork/core/macro.hpp"

/**
 * @file numa.hpp
 *
 * @brief An abstraction over `hwloc`.
 */

#ifdef LF_HAS_HWLOC
  #include <hwloc.h>
#else
#endif

/**
 * @brief An opaque description of a set of processing units.
 *
 * \rst
 *
 * .. note::
 *
 *    If your system is missing `hwloc` then this type will be incomplete.
 *
 * \endrst
 */
struct hwloc_bitmap_s;

/**
 * @brief An opaque description of a computers topology.
 *
 * \rst
 *
 * .. note::
 *
 *    If your system is missing `hwloc` then this type will be incomplete.
 *
 * \endrst
 */
struct hwloc_topology;

namespace lf {

inline namespace ext {

// ------------- hwloc can go wrong in a lot of ways... ------------- //

/**
 * @brief An exception thrown when `hwloc` fails.
 */
struct hwloc_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// ------------------------------ Topology decl ------------------------------
// //

/**
 * @brief Enum to control distribution strategy of workers among numa nodes.
 */
enum class numa_strategy {
  fan, // Put workers as far away from each other as possible (maximize cache.)
  seq, // Fill up each numa node sequentially (ignoring SMT).
};

/**
 * @brief A shared description of a computers topology.
 *
 * Objects of this type have shared-pointer semantics.
 */
class numa_topology {

  struct bitmap_deleter {
    LF_STATIC_CALL void operator()(hwloc_bitmap_s *ptr) LF_STATIC_CONST noexcept {
#ifdef LF_HAS_HWLOC
      hwloc_bitmap_free(ptr);
#else
      LF_ASSERT(!ptr);
#endif
    }
  };

  using unique_cpup = std::unique_ptr<hwloc_bitmap_s, bitmap_deleter>;

  using shared_topo = std::shared_ptr<hwloc_topology>;

 public:
  /**
   * @brief Construct a topology.
   *
   * If `hwloc` is not installed this topology is empty.
   */
  numa_topology();

  /**
   * @brief Test if this topology is empty.
   */
  explicit operator bool() const noexcept { return m_topology != nullptr; }

  /**
   * A handle to a single processing unit in a NUMA computer.
   */
  class numa_handle {
   public:
    /**
     * @brief Bind the calling thread to the set of processing units in this
     * `cpuset`.
     *
     * If `hwloc` is not installed both handles are null and this is a noop.
     */
    void bind() const;

    shared_topo topo = nullptr; ///< A shared handle to topology this handle belongs to.
    unique_cpup cpup = nullptr; ///< A unique handle to processing units in
                                ///< `topo` that this handle represents.
  };

  /**
   * @brief Split a topology into `n` uniformly distributed handles to single
   * processing units.
   *
   * Here "uniformly" means we try to use the minimum number of numa nodes then
   * divided each node such that each PU has as much cache as possible. If this
   * topology is empty then this function returns a vector of `n` empty handles.
   */
  auto split(std::size_t n, numa_strategy strategy = numa_strategy::seq) const -> std::vector<numa_handle>;

  /**
   * @brief A single-threads hierarchical view of a set of objects.
   *
   * This is a `numa_handle` augmented with  list of neighbors-lists each
   * neighbors-list has equidistant neighbors. The first neighbors-list always
   * exists and contains only one element, the one "owned" by the thread. Each
   * subsequent neighbors-list has elements that are topologically more distant
   * from the element in the first neighbour-list.
   */
  template <typename T>
  struct numa_node : numa_handle {
    /**
     * @brief A list of neighbors-lists.
     */
    std::vector<std::vector<std::shared_ptr<T>>> neighbors;
  };

  /**
   * @brief Distribute a vector of objects over this topology.
   *
   * This function returns a vector of `numa_node`s. Each `numa_node` contains a
   * hierarchical view of the elements in `data`.
   */
  template <typename T>
  auto distribute(std::vector<std::shared_ptr<T>> const &data, numa_strategy strategy = numa_strategy::seq)
      -> std::vector<numa_node<T>>;

 private:
  shared_topo m_topology = nullptr;
};

// ------------------------------ Topology implementation
// ------------------------------ //

#ifdef LF_HAS_HWLOC

inline numa_topology::numa_topology() {

  struct topology_deleter {
    LF_STATIC_CALL void operator()(hwloc_topology *ptr) LF_STATIC_CONST noexcept {
      if (ptr != nullptr) {
        hwloc_topology_destroy(ptr);
      }
    }
  };

  hwloc_topology *tmp = nullptr;

  if (hwloc_topology_init(&tmp) != 0) {
    LF_THROW(hwloc_error{"failed to initialize a topology"});
  }

  m_topology = {tmp, topology_deleter{}};

  if (hwloc_topology_load(m_topology.get()) != 0) {
    LF_THROW(hwloc_error{"failed to load a topology"});
  }
}

inline void numa_topology::numa_handle::bind() const {

  LF_ASSERT(topo);
  LF_ASSERT(cpup);

  switch (hwloc_set_cpubind(topo.get(), cpup.get(), HWLOC_CPUBIND_THREAD)) {
    case 0:
      return;
    case -1:
      switch (errno) {
        case ENOSYS:
          LF_THROW(hwloc_error{"hwloc's cpu binding is not supported on this system"});
        case EXDEV:
          LF_THROW(hwloc_error{"hwloc cannot enforce the requested binding"});
        default:
          LF_THROW(hwloc_error{"hwloc cpu bind reported an unknown error"});
      };
    default:
      LF_THROW(hwloc_error{"hwloc cpu bind returned un unexpected value"});
  }
}

inline auto count_cores(hwloc_obj_t obj) -> unsigned int {

  LF_ASSERT(obj);

  if (obj->type == HWLOC_OBJ_CORE) {
    return 1;
  }

  unsigned int num_cores = 0;

  for (unsigned int i = 0; i < obj->arity; i++) {
    num_cores += count_cores(obj->children[i]);
  }

  return num_cores;
}

inline auto numa_topology::split(std::size_t n, numa_strategy strategy) const -> std::vector<numa_handle> {

  if (n < 1) {
    LF_THROW(hwloc_error{"hwloc cannot distribute over less than one singlet"});
  }

  // We are going to build up a list of numa packages until we have enough
  // cores.

  std::vector<hwloc_obj_t> roots;

  if (strategy == numa_strategy::seq) {

    hwloc_obj_t numa = nullptr;

    for (unsigned int count = 0; count < n; count += count_cores(numa)) {

      hwloc_obj_t next_numa = hwloc_get_next_obj_by_type(m_topology.get(), HWLOC_OBJ_PACKAGE, numa);

      if (next_numa == nullptr) {
        break;
      }

      roots.push_back(next_numa);
      numa = next_numa;
    }
  } else {
    roots.push_back(hwloc_get_root_obj(m_topology.get()));
  }

  // Now we distribute over the cores in each numa package, NOTE:  hwloc_distrib
  // gives us owning pointers (not in the docs, but it does!).

  std::vector<hwloc_bitmap_s *> sets(n, nullptr);

  if (hwloc_distrib(m_topology.get(), roots.data(), roots.size(), sets.data(), sets.size(), INT_MAX, 0) !=
      0) {
    LF_THROW(hwloc_error{"unknown hwloc error when distributing over a topology"});
  }

  // Need ownership before map for exception safety.
  std::vector<unique_cpup> singlets{sets.begin(), sets.end()};

  return impl::map(std::move(singlets), [&](unique_cpup &&singlet) -> numa_handle {
    //
    if (!singlet) {
      LF_THROW(hwloc_error{"hwloc_distrib returned a nullptr"});
    }

    if (hwloc_bitmap_singlify(singlet.get()) != 0) {
      LF_THROW(hwloc_error{"unknown hwloc error when singlify a bitmap"});
    }

    return {m_topology, std::move(singlet)};
  });
}

namespace detail {

class distance_matrix {

  using numa_handle = numa_topology::numa_handle;

 public:
  /**
   * @brief Compute the topological distance between all pairs of objects in
   * `obj`.
   */
  explicit distance_matrix(std::vector<numa_handle> const &handles)
      : m_size{handles.size()},
        m_matrix(m_size * m_size) {

    // Transform into hwloc's internal representation of nodes in the topology
    // tree.

    std::vector obj = impl::map(handles, [](numa_handle const &handle) -> hwloc_obj_t {
      return hwloc_get_obj_covering_cpuset(handle.topo.get(), handle.cpup.get());
    });

    for (auto *elem : obj) {
      if (elem == nullptr) {
        LF_THROW(hwloc_error{"failed to find an object covering a handle"});
      }
    }

    // Build the matrix.

    for (std::size_t i = 0; i < obj.size(); i++) {
      for (std::size_t j = 0; j < obj.size(); j++) {

        auto *topo_1 = handles[i].topo.get();
        auto *topo_2 = handles[j].topo.get();

        if (topo_1 != topo_2) {
          LF_THROW(hwloc_error{"numa_handles are in different topologies"});
        }

        hwloc_obj_t ancestor = hwloc_get_common_ancestor_obj(topo_1, obj[i], obj[j]);

        if (ancestor == nullptr) {
          LF_THROW(hwloc_error{"failed to find a common ancestor"});
        }

        int dist_1 = obj[i]->depth - ancestor->depth;
        int dist_2 = obj[j]->depth - ancestor->depth;

        LF_ASSERT(dist_1 >= 0);
        LF_ASSERT(dist_2 >= 0);

        m_matrix[i * m_size + j] = std::max(dist_1, dist_2);
      }
    }
  }

  auto operator()(std::size_t i, std::size_t j) const noexcept -> int { return m_matrix[i * m_size + j]; }

  auto size() const noexcept -> std::size_t { return m_size; }

 private:
  std::size_t m_size;
  std::vector<int> m_matrix;
};

} // namespace detail

template <typename T>
inline auto numa_topology::distribute(std::vector<std::shared_ptr<T>> const &data, numa_strategy strategy)
    -> std::vector<numa_node<T>> {

  std::vector handles = split(data.size(), strategy);

  // Compute the topological distance between all pairs of objects.

  detail::distance_matrix dist{handles};

  std::vector<numa_node<T>> nodes = impl::map(std::move(handles), [](numa_handle &&handle) -> numa_node<T> {
    return {std::move(handle), {}};
  });

  // Compute the neighbors-lists.

  for (std::size_t i = 0; i < nodes.size(); i++) {

    std::set<int> uniques;

    for (std::size_t j = 0; j < nodes.size(); j++) {
      if (i != j) {
        uniques.insert(dist(i, j));
      }
    }

    nodes[i].neighbors.resize(1 + uniques.size());

    for (std::size_t j = 0; j < nodes.size(); j++) {
      if (i == j) {
        nodes[i].neighbors[0].push_back(data[j]);
      } else {
        auto idx = std::distance(uniques.begin(), uniques.find(dist(i, j)));
        LF_ASSERT(idx >= 0);
        nodes[i].neighbors[1 + static_cast<std::size_t>(idx)].push_back(data[j]);
      }
    }
  }

  return nodes;
}

#else

inline numa_topology::numa_topology()
    : m_topology{nullptr, [](hwloc_topology *ptr) {
                   LF_ASSERT(!ptr);
                 }} {}

inline void numa_topology::numa_handle::bind() const {
  LF_ASSERT(!topo);
  LF_ASSERT(!cpup);
}

inline auto numa_topology::split(std::size_t n, numa_strategy /* strategy */) const
    -> std::vector<numa_handle> {
  return std::vector<numa_handle>(n);
}

template <typename T>
inline auto numa_topology::distribute(std::vector<std::shared_ptr<T>> const &data, numa_strategy strategy)
    -> std::vector<numa_node<T>> {

  std::vector<numa_handle> handles = split(data.size(), strategy);

  std::vector<numa_node<T>> views;

  for (std::size_t i = 0; i < data.size(); i++) {

    numa_node<T> node{
        std::move(handles[i]), {{data[i]}}, // The first neighbors-list contains
                                            // only the object itself.
    };

    if (data.size() > 1) {
      node.neighbors.push_back({});
    }

    for (auto const &neigh : data) {
      if (neigh != data[i]) {
        node.neighbors[1].push_back(neigh);
      }
    }

    views.push_back(std::move(node));
  }

  return views;
}

#endif

} // namespace ext

} // namespace lf

#endif /* D8877F11_1F66_4AD0_B949_C0DFF390C2DB */
