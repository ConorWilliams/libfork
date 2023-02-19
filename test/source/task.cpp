// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <numeric>
#include <span>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define NLOG
// #define NDEBUG

#include "libfork/basic_task.hpp"
#include "libfork/inline.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

template <typename T>
using task = basic_task<T, inline_context>;

// task<int> fib(int x) {
//   //
//   if (x < 2) {
//     co_return x;
//   }

//   future<int> a, b;

//   co_await fork(a, fib, x - 1);
//   co_await just(b, fib, x - 2);

//   co_await join();

//   co_return a + b;
// }

task<void> noop(int) {
  co_return;
}

// TEST_CASE("Basic task manipulation", "[task]") {
//   //

//   inline_context context{};

//   REQUIRE(sync_wait(context, fib(0)) == 0);
//   REQUIRE(sync_wait(context, fib(1)) == 1);
//   REQUIRE(sync_wait(context, fib(2)) == 1);
//   REQUIRE(sync_wait(context, fib(3)) == 2);
//   REQUIRE(sync_wait(context, fib(4)) == 3);
//   REQUIRE(sync_wait(context, fib(5)) == 5);
// }

// NOLINTEND