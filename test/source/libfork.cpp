// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>
#include <iostream>
#include <memory>
#include <new>
#include <semaphore>
#include <stack>
#include <type_traits>
#include <utility>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#define NDEBUG
// #define LIBFORK_PROPAGATE_EXCEPTIONS
// #undef LIBFORK_LOG
// #define LIBFORK_LOGGING

#include "libfork/libfork.hpp"
#include "libfork/macro.hpp"
#include "libfork/queue.hpp"
#include "libfork/schedule/inline.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

inline constexpr auto fib = fn([](auto self, int n) -> task<int> {
  //
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, self)(n - 1);
  co_await lf::call(b, self)(n - 2);

  co_await lf::join;

  co_return a + b;
});

int fib_(int n) {
  if (n < 2) {
    return n;
  }

  return fib_(n - 1) + fib_(n - 2);
}

// TEST_CASE("bench") {

//   volatile int x = 20;

//   basic_context ctx;

//   basic_context::set(ctx);

//   BENCHMARK("no coro") {
//     return fib_(x);
//   };

//   BENCHMARK("inline") {
//     // return fib_(x);
//     return sync_wait([](auto handle) { handle(); }, fib, x);
//   };
// }

// class my_class {
// public:
//   static constexpr auto get = [](my_class &) -> task<int> {
//     // co_return self->m_private;
//   };

// private:
//   int m_private = 99;
// };

// TEST_CASE("access", "[access]") {
//   auto exec = [](auto handle) { handle(); };

//   my_class obj;

//   // sync_wait(exec, my_class::access, obj);
// }

TEST_CASE("libfork", "[libfork]") {

  inline_scheduler schedule;

  int i = 22;

  // my_class obj;

  // // REQUIRE(99 == sync_wait([](auto handle) { handle(); }, my_class::get, obj));

  auto answer = sync_wait(schedule, fib, i);

  REQUIRE(answer == fib_(i));

  std::cout << "fib(" << i << ") = " << answer << "\n";
}

// NOLINTEND