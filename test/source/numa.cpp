

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #undef LF_HAS_HWLOC

#ifndef LF_HAS_HWLOC
  #define LF_HAS_HWLOC
#endif

#include <iostream>
#include <memory>
#include <set>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"

#include "libfork/schedule/numa.hpp"

using namespace lf;

using namespace impl;

TEST_CASE("make_topology", "[numa]") {
  for (int i = 0; i < 10; i++) {
    numa_topology topo = {};

#ifdef LF_HAS_HWLOC
    REQUIRE(topo);
#else
    REQUIRE(!topo);
#endif
  }
}

#ifdef LF_HAS_HWLOC

namespace {

struct comp {
  auto operator()(numa_topology::numa_handle const &lhs, numa_topology::numa_handle const &rhs) const noexcept -> bool {
    return hwloc_bitmap_compare(lhs.cpup.get(), rhs.cpup.get()) < 0;
  }
};

} // namespace

TEST_CASE("split", "[numa]") {

  numa_topology topo;

  std::size_t max_unique = std::thread::hardware_concurrency();

  for (std::size_t i = 1; i < 2 * max_unique; i++) {

    std::vector<numa_topology::numa_handle> singlets = topo.split(i);

    REQUIRE(singlets.size() == i);

    std::set<numa_topology::numa_handle, comp> unique_bitmaps;

    for (auto &singlet : singlets) {
      unique_bitmaps.emplace(std::move(singlet));
    }

    if (i < max_unique) {
      REQUIRE(unique_bitmaps.size() == i);
    } else {
      REQUIRE(unique_bitmaps.size() == max_unique);
    }
  }
}

namespace {

void print_distances(lf::ext::detail::distance_matrix const &dist) {

  std::cout << "distances [" << dist.size() << "." << dist.size() << "]:\n";

  for (std::size_t i = 0; i < dist.size(); i++) {
    for (std::size_t j = 0; j < dist.size(); j++) {
      std::cout << dist(i, j) << " ";
    }
    std::cout << std::endl;
  }
}

} // namespace

TEST_CASE("distances", "[numa]") {

  numa_topology topo;

  std::size_t max_unique = std::thread::hardware_concurrency();

  for (std::size_t i = 1; i <= 2 * max_unique; i++) {

    ext::detail::distance_matrix dist{topo.split(i)};

    REQUIRE(dist.size() == i);

    print_distances(dist);

    for (std::size_t i = 0; i < dist.size(); i++) {
      for (std::size_t j = 0; j < dist.size(); j++) {
        REQUIRE(dist(i, j) >= 0);
        REQUIRE(dist(i, j) == dist(j, i));
        if (i == j) {
          REQUIRE(dist(i, j) == 0);
        }
      }
    }
  }
}

#endif

TEST_CASE("distribute", "[numa]") {

  for (unsigned int i = 1; i <= 2 * std::thread::hardware_concurrency(); i++) {

    std::vector<std::shared_ptr<unsigned int>> ints;

    for (unsigned int j = 0; j < i; j++) {
      ints.push_back(std::make_shared<unsigned int>(j));
    }

    numa_topology topo{};

    std::vector views = topo.distribute(ints);

    REQUIRE(views.size() == i);

    unsigned int count = 0;

    for (auto &&node : views) {

      REQUIRE(!node.neighbors.empty());
      REQUIRE(node.neighbors.front().size() == 1);
      REQUIRE(*node.neighbors.front().front() == count++);

      std::size_t sum = 0;

      for (auto &&nl : node.neighbors) {
        sum += nl.size();
      }

      REQUIRE(sum == ints.size());
    }

    std::cout << "View from the first topo:";

    for (auto &&nl : views.front().neighbors) {
      std::cout << " " << nl.size();
    }

    std::cout << std::endl;
  }
}
