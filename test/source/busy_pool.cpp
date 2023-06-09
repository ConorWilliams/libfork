// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>
#include <memory>
#include <new>
#include <semaphore>
#include <stack>
#include <type_traits>
#include <utility>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define LIBFORK_LOGGING

#include "libfork/busy_pool.hpp"
#include "libfork/libfork.hpp"

// NOLINTBEGIN

using namespace lf;

template <typename T>
using ptask = task<T, typename lf::detail::busy_pool::worker_context>;

inline constexpr auto fib = fn([](auto fib, int n) -> ptask<int> {
  //
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, fib)(n - 1);
  co_await lf::call(b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
});

inline int fib_2(int n) {
  if (n < 2) {
    return n;
  }

  return fib_2(n - 1) + fib_2(n - 2);
};

TEST_CASE("fib_2") {

  BENCHMARK("classic") {
    return fib_2(25);
  };

  BENCHMARK("pool") {
    detail::busy_pool pool{2};

    LIBFORK_LOG("iter");
    auto x = sync_wait(pool.schedule(), fib, 0);

    LIBFORK_LOG("psot");

    return x;
  };
}

TEST_CASE("busy_pool", "[libfork]") {

  for (int i = 0; i < 20; ++i) {

    detail::busy_pool pool{};

    auto result = sync_wait(pool.schedule(), fib, i);

    REQUIRE(result == fib_2(i));
  }
}

// NOLINTEND