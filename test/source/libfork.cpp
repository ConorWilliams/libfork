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

#include <catch2/catch_template_test_macros.hpp>
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

int fib(int n) {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

inline constexpr auto r_fib = fn([](auto fib, int n) -> lf::task<int> {
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

inline constexpr auto v_fib = fn([](auto fib, int &ret, int n) -> lf::task<void> {
  //
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  co_await lf::fork(fib)(a, n - 1);
  co_await lf::call(fib)(b, n - 2);

  co_await lf::join;

  ret = a + b;
});

class access {

public:
  static constexpr auto get = mem_fn([](auto self) -> lf::task<int> {
    co_await lf::call(self->run)(*self, 10);
    co_return self->m_private;
  });

private:
  static constexpr auto run = mem_fn([](auto self, int n) -> lf::task<> {
    if (n < 0) {
      co_return;
    }
    co_await lf::call(self->run)(*self, n - 1);
  });

  int m_private = 99;
};

inline constexpr auto mem_from_coro = fn([](auto self) -> lf::task<int> {
  access a;
  int r;
  co_await lf::call(r, access::get)(a);
  co_return r;
});

struct deep : std::exception {};

inline constexpr auto deep_except = fn([](auto self, int n) -> lf::task<> {
  if (n > 0) {
    co_await lf::call(self)(n - 1);
  }
  throw deep{};
});

template <scheduler S>
void test(S &schedule) {
  SECTION("Fibonacci") {
    for (int i = 0; i < 25; ++i) {
      REQUIRE(fib(i) == sync_wait(schedule, r_fib, i));
    }
  }
  SECTION("Void Fibonacci") {
    int res;

    for (int i = 0; i < 25; ++i) {
      sync_wait(schedule, v_fib, res, i);
      REQUIRE(fib(i) == res);
    }
  }
  SECTION("member function") {
    access a;
    REQUIRE(99 == sync_wait(schedule, access::get, a));
    REQUIRE(99 == sync_wait(schedule, mem_from_coro));
  }

#if LIBFORK_PROPAGATE_EXCEPTIONS
  SECTION("exception propagate") {
    REQUIRE_THROWS_AS(sync_wait(schedule, deep_except, 10), deep);
  }
#endif
}

TEMPLATE_TEST_CASE("libfork", "[libfork][template]", inline_scheduler) {
  for (int i = 0; i < 10; ++i) {
    TestType schedule{};
    test(schedule);
  }
}

// NOLINTEND