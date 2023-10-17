// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #define NDEBUG
// #define LF_COROUTINE_OFFSET 2 * sizeof(void *)

#include <list>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/algorithm/for_each.hpp"
#include "libfork/schedule.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

template <typename T>
void check(T const &v, int k) {
  for (int i = 0; auto &&elem : v) {
    REQUIRE(elem == i++ + k);
  }
}

TEMPLATE_TEST_CASE("for each", "[algorithm][template]", std::vector<int>, std::list<int>) {

  int count = 0;

  TestType v;

  for (auto i = 0; i < 10'000; i++) {
    v.push_back(i);
  }

  check(v, count++);

  lf::lazy_pool pool{};

  // --------------- First regular function --------------- //

  {
    auto fun = [](int &i) {
      i++;
    };

    // Check grain = 1 case:
    lf::sync_wait(pool, lf::for_each, v, fun);
    check(v, count++);

    // Check grain > 1 and n % grain == 0 case:

    REQUIRE(v.size() % 100 == 0);
    lf::sync_wait(pool, lf::for_each, v, 100, fun);
    check(v, count++);

    // Check grain > 1 and n % grain != 0 case:
    REQUIRE(v.size() % 300 != 0);
    lf::sync_wait(pool, lf::for_each, v, 300, fun);
    check(v, count++);

    // Check grain > size case:
    lf::sync_wait(pool, lf::for_each, v, 20'000, fun);
    check(v, count++);
  }

  // --------------- Now async --------------- //

  {
    async fun = [](auto, int &i) -> task<> {
      i++;
      co_return;
    };

    // Check grain = 1 case:
    lf::sync_wait(pool, lf::for_each, v, fun);
    check(v, count++);

    // Check grain > 1 and n % grain == 0 case:

    REQUIRE(v.size() % 100 == 0);
    lf::sync_wait(pool, lf::for_each, v, 100, fun);
    check(v, count++);

    // Check grain > 1 and n % grain != 0 case:
    REQUIRE(v.size() % 300 != 0);
    lf::sync_wait(pool, lf::for_each, v, 300, fun);
    check(v, count++);

    // Check grain > size case:
    lf::sync_wait(pool, lf::for_each, v, 20'000, fun);
    check(v, count++);
  }
}

// NOLINTEND