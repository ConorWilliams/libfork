// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #define NDEBUG
// #define LF_COROUTINE_OFFSET 2 * sizeof(void *)

#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for INTERNAL_CATCH_NOINTERNAL_CATCH_DEF
#include <concepts>                              // for constructible_from
#include <cstddef>                               // for size_t
#include <functional>                            // for identity
#include <span>                                  // for span
#include <thread>                                // for thread
#include <utility>                               // for forward
#include <vector>                                // for vector, allocator, operator==

#include "libfork/algorithm/map.hpp" // for map
#include "libfork/core.hpp"          // for sync_wait, task
#include "libfork/schedule.hpp"      // for unit_pool, busy_pool, lazy_pool

// NOLINTBEGIN No linting in tests

using namespace lf;

namespace {

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

template <typename T>
void check(T const &v, int k) {
  for (int i = 0; auto &&elem : v) {
    REQUIRE(elem == i++ + k);
  }
}

template <typename Sch, typename F, typename Proj = std::identity>
void test(Sch &&sch, F add_one, Proj proj = {}) {
  //
  int count = 0;

  std::vector<int> v;

  for (auto i = 0; i < 10'000; i++) {
    v.push_back(i);
  }

  check(v, count++);

  {
    // Check grain = 1 case:
    lf::sync_wait(sch, lf::map, v, v.begin(), add_one, proj);
    check(v, count++);

    // Check grain > 1 and n % grain == 0 case:

    REQUIRE(v.size() % 100 == 0);
    lf::sync_wait(sch, lf::map, v, v.begin(), 100, add_one);
    check(v, count++);

    // Check grain > 1 and n % grain != 0 case:
    REQUIRE(v.size() % 300 != 0);
    lf::sync_wait(sch, lf::map, v, v.begin(), 300, add_one);
    check(v, count++);

    // Check grain > size case:
    REQUIRE(v.size() < 20'000);
    lf::sync_wait(sch, lf::map, v, v.begin(), 20'000, add_one);
    check(v, count++);
  }

#ifndef _MSC_VER

  // ----------- Now with small inputs ----------- //

  for (int i = 1; i < 4; ++i) {

    std::vector<int> small{0, 0, 0};

    lf::sync_wait(sch, lf::map, std::span(small.data(), 3), small.begin(), i, add_one);
    REQUIRE(small == std::vector<int>{1, 1, 1});

    lf::sync_wait(sch, lf::map, std::span(small.data(), 2), small.begin(), i, add_one);
    REQUIRE(small == std::vector<int>{2, 2, 1});

    lf::sync_wait(sch, lf::map, std::span(small.data(), 1), small.begin(), i, add_one);
    REQUIRE(small == std::vector<int>{3, 2, 1});

    lf::sync_wait(sch, lf::map, std::span(small.data(), 0), small.begin(), i, add_one);
    REQUIRE(small == std::vector<int>{3, 2, 1});
  }

#endif
}

constexpr auto add_reg = [](int const &i) -> int {
  return i + 1;
};

constexpr auto add_coro = [](auto, int const &i) -> task<int> {
  co_return i + 1;
};

constexpr auto coro_identity = []<typename T>(auto, T &&val) -> task<T &&> {
  co_return std::forward<T>(val);
};

} // namespace

TEMPLATE_TEST_CASE("map (reg, reg)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), add_reg);
}

TEMPLATE_TEST_CASE("map (co, reg)", "[algorithm][template]", unit_pool /*, busy_pool, lazy_pool*/) {
  test(make_scheduler<TestType>(), add_coro);
}

TEMPLATE_TEST_CASE("map (reg, co)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), add_reg, coro_identity);
}

TEMPLATE_TEST_CASE("map (co, co)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), add_coro, coro_identity);
}

// NOLINTEND