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

#include "libfork/core.hpp"     // for sync_wait, detach, LF_CATCH_ALL, LF_TRY
#include "libfork/schedule.hpp" // for unit_pool, busy_pool, lazy_pool

// NOLINTBEGIN No linting in tests

using namespace lf;

namespace {

auto sfib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return sfib(n - 1) + sfib(n - 2);
}

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

struct fib {

  int count = 0;

  fib() = default;

  fib(fib const &other) : count(other.count + 1) {
    if (count > 50) {
      LF_THROW(std::runtime_error("Too many copies"));
    }
  }

  auto operator()(auto fib, int n) -> lf::task<int> {
    //
    if (n < 2) {
      co_return n;
    }

    int a, b;

    // clang-format off

    LF_TRY {
      impl::ignore_t{} = lf::fork(&a, fib);        // Not calling.
      impl::ignore_t{} = lf::fork(&a, fib)(n - 2); // Not co_awaiting.
    } LF_CATCH_ALL {
      //.. sink any exceptions
    }

    co_await lf::fork(&a, fib)(n - 1);

    LF_TRY {
        impl::ignore_t{} = lf::call(&a, fib)(n - 2); // Ok as immediately destructed.

        co_await lf::call(&b, fib)(n - 2);
    } LF_CATCH_ALL {
        fib.stash_exception();
    }

    // clang-format on

    co_await lf::join;

    auto _ = lf::fork(&a, fib)(n - 1); // Keeping the result is ok as we destruct on the same worker.

    co_return a + b;
  }
};

} // namespace

TEMPLATE_TEST_CASE("Bad user behavior", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 25; ++j) {

      // clang-format off
        
      LF_TRY {
        auto res = lf::sync_wait(sch, fib{}, j);
        REQUIRE(res == sfib(j));
      } LF_CATCH_ALL {
        // OK if the test threw
      }

      // clang-format on
    }
  }
}

// TODO, experiment with detach here a bit more.

TEMPLATE_TEST_CASE("Bad user behavior, detached", "[core][template]", unit_pool) {

  auto sch = make_scheduler<TestType>();

  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 25; ++j) {
      lf::detach(sch, fib{}, j);
    }
  }
}

namespace {

struct noop {

  int count = 0;

  noop() = default;

  noop(noop const &other) : count(other.count + 1) {
    if (count > 1) {
      LF_THROW(std::runtime_error("Too many copies"));
    }
  }

  auto operator()(auto) -> lf::task<> { co_return; }
};

} // namespace

TEMPLATE_TEST_CASE("Throw in sync wait", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  // clang-format off

  auto foo = noop{};

  LF_TRY {
    lf::sync_wait(sch, foo);
    REQUIRE(false);
  } LF_CATCH_ALL {
    // Expect the test to throw.
  }

  // clang-format on
}

TEMPLATE_TEST_CASE("Throw in sync detach", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  // clang-format off

  auto foo = noop{};

  LF_TRY {
    detach(sch, foo);
    REQUIRE(false);
  } LF_CATCH_ALL {
    // Expect the test to throw.
  }

  // clang-format on
}

// NOLINTEND No linting in tests