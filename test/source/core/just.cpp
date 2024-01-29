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
#include <stdexcept>                             // for runtime_error
#include <thread>                                // for thread

#include "libfork/core.hpp"     // for sync_wait, task, call_just, LF_CATCH_ALL
#include "libfork/schedule.hpp" // for busy_pool, lazy_pool, unit_pool

// NOLINTBEGIN No linting in tests

using namespace lf;

// ------------------------ Construct destruct ------------------------ //

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

inline constexpr auto r_fib = [](auto fib, int n) -> lf::task<int> {
  //

  if (n < 2) {
    co_return n;
  }

  int a = co_await just(fib)(n - 1);
  int b = co_await just(::fib)(n - 2);

  co_return a + b;
};

} // namespace

TEMPLATE_TEST_CASE("Just", "[just][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  for (int i = 0; i < 20; ++i) {
    REQUIRE(sync_wait(sch, r_fib, i) == fib(i));
  }
}

namespace {

constexpr auto exception = [](auto) -> task<void> {
  LF_THROW(std::runtime_error("exception"));
  co_return;
};

constexpr auto exception_call = [](auto) -> task<bool> {
  //
  // clang-format off

  LF_TRY {
    co_await just(exception)();
    co_return false;
  } LF_CATCH_ALL {
    co_return true;
  }

  // clang-format on
};

} // namespace

#if LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE("Exceptionally just", "[just][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  REQUIRE(sync_wait(sch, exception_call));
}

#endif
