// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for INTERNAL_CATCH_NOINTERNAL_CATCH_DEF
#include <catch2/catch_test_macros.hpp>          // for operator<, operator>=, AssertionHandler
#include <concepts>                              // for constructible_from
#include <cstddef>                               // for size_t
#include <exception>                             // for exception_ptr, current_exception, opera...
#include <stdexcept>                             // for runtime_error
#include <thread>                                // for thread
#include <utility>                               // for move

#include "libfork/core.hpp"     // for sync_wait, task, LF_CATCH_ALL, LF_COMPI...
#include "libfork/schedule.hpp" // for busy_pool, lazy_pool, unit_pool

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

namespace {

using lf::call;
using lf::fork;
using lf::join;

using lf::task;

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

constexpr auto fib = [](auto fib, int n) -> task<int> {
  //

  if (n == 7 && LF_COMPILER_EXCEPTIONS) {
    LF_THROW(std::runtime_error{"7 is unlucky"});
  }

  if (n < 2) {
    co_return n;
  }

  int a, b;

  {
    std::exception_ptr exception_ptr;

    // clang-format off

    LF_TRY {
      co_await fork(&a, fib)(n - 1);
      co_await call(&b, fib)(n - 2);
      goto fallthrough;
    } LF_CATCH_ALL {
      exception_ptr = std::current_exception();
    }

    // clang-format on

    co_await join;
    std::rethrow_exception(std::move(exception_ptr));

  fallthrough:
    LF_ASSERT(exception_ptr == nullptr);
    co_await join;
  }

  co_return a + b;
};

constexpr auto fib_integ = [](auto fib, int n) -> task<int> {
  //

  if (n == 7 && LF_COMPILER_EXCEPTIONS) {
    LF_THROW(std::runtime_error{"7 is unlucky"});
  }

  if (n < 2) {
    co_return n;
  }

  int a, b;

  // clang-format off

  LF_TRY {
    co_await fork(&a, fib)(n - 1);
    co_await call(&b, fib)(n - 2);
  } LF_CATCH_ALL {
    fib.stash_exception();
  }

  // clang-format on

  // This rethrows the exception if there was one.
  co_await join;

  co_return a + b;
};

} // namespace

// #ifdef LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE("Exceptional fib", "[exception][template]", unit_pool, busy_pool, lazy_pool) {

  auto schedule = make_scheduler<TestType>();

  for (int i = 0; i < 1000; ++i) {

    for (int j = 0; j < 14; ++j) {

      // clang-format off

      LF_TRY {
        sync_wait(schedule, fib, j);
        
        #if LF_COMPILER_EXCEPTIONS
          REQUIRE(j < 7);
        #endif
      } LF_CATCH_ALL {
        REQUIRE(j >= 7);
      }

      // clang-format on
    }
  }
}

TEMPLATE_TEST_CASE("Integ exceptional fib", "[exception][template]", unit_pool, busy_pool, lazy_pool) {

  auto schedule = make_scheduler<TestType>();

  for (int i = 0; i < 1000; ++i) {

    for (int j = 0; j < 14; ++j) {

      // clang-format off

      LF_TRY {
        
        sync_wait(schedule, fib_integ, j);

        #if LF_COMPILER_EXCEPTIONS
          REQUIRE(j < 7);
        #endif

      } LF_CATCH_ALL {
        REQUIRE(j >= 7);
      }

      // clang-format on
    }
  }
}

// NOLINTEND