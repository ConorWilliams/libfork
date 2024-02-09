// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for INTERNAL_CATCH_NOINTERNAL_CATCH_DEF
#include <concepts>                              // for constructible_from
#include <cstddef>                               // for size_t
#include <thread>                                // for thread

#include "extern.hpp"
#include "libfork/core.hpp"     // for sync_wait
#include "libfork/schedule.hpp" // for busy_pool, lazy_pool, unit_pool

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

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

} // namespace

TEMPLATE_TEST_CASE("Fibonacci externed", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  auto schedule = make_scheduler<TestType>();

  for (int j = 0; j < 100; ++j) {
    for (int i = 1; i < 20; ++i) {
      REQUIRE(fib(i) == sync_wait(schedule, externed_fib, i));
    }
  }
}

TEMPLATE_TEST_CASE("Reference externed", "[core][template]", unit_pool, busy_pool, lazy_pool) {
  //
  auto schedule = make_scheduler<TestType>();

  for (int i = 1; i < 100; ++i) {
    REQUIRE(&i == &sync_wait(schedule, externed_ref, i));
  }
}