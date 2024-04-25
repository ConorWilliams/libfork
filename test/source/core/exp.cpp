// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for INTERNAL_CATCH_NOINTERNAL_CATCH_DEF

#include "libfork/core.hpp"     // for sync_wait, task, LF_CATCH_ALL, LF_COMPI...
#include "libfork/schedule.hpp" // for busy_pool, lazy_pool, unit_pool

#include "libfork/experimental/task.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

namespace {

auto fob(int n) -> experimental::task<int> {

  if (n < 2) {
    co_return n;
  }

  int a = 0, b = 0;

  auto sc = co_await experimental::make_scope_t{};

  co_await sc.fork(&a, fob, n - 1);
  co_await sc.call(&b, fob, n - 2);

  co_await sc;

  co_return a + b;
}

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

} // namespace

TEST_CASE("Experimental", "[exp][template]") {
  for (int j = 0; j < 100; ++j) {
    //
    auto schedule = experimental::unit_pool{};

    for (int i = 1; i < 20; ++i) {
      REQUIRE(fib(i) == experimental::schedule(schedule, fob, i));
    }
  }
}