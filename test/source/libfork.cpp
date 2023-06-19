// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <semaphore>
#include <stack>
#include <type_traits>
#include <utility>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

// #define NDEBUG
// #define LIBFORK_PROPAGATE_EXCEPTIONS
// #undef LIBFORK_LOG
// #define LIBFORK_LOGGING

#include "libfork/libfork.hpp"
#include "libfork/macro.hpp"
#include "libfork/queue.hpp"
#include "libfork/schedule/busy.hpp"
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

  REQUIRE(fib.context().max_threads() >= 0);

  if (n < 2) {
    co_return n;
  }

  int a, b;

  {
    std::exception_ptr ptr;

    try {
      // If we leave this scope by exception or otherwise, we must
      // make sure join is called before a,b destructed.
      co_await lf::fork(a, fib)(n - 1);
      co_await lf::call(b, fib)(n - 2);
    } catch (...) {
      ptr = std::current_exception();
    }

    co_await lf::join;

    if (ptr) {
      std::rethrow_exception(ptr);
    }
  }

  co_return a + b;
});

inline constexpr auto v_fib = fn([](auto fib, int &ret, int n) -> lf::task<void> {
  //
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  for (int i = 0; i < 2; i++) {
    co_await lf::fork(fib)(a, n - 1);
    co_await lf::call(fib)(b, n - 2);
    co_await lf::join;
  }

  ret = a + b;
});

inline constexpr auto r_fib_2 = fn([](auto fib, int n) -> lf::task<int> {
  //

  if (n < 2) {
    co_return n;
  }

  int a = co_await fib(n - 1);
  int b = co_await fib(n - 2);

  co_return a + b;
});

class access_test {

public:
  static constexpr auto get = mem_fn([](auto self) -> lf::task<int> {
    co_await lf::call(self->run)(*self, 10);
    co_await lf::join;
    co_return self->m_private;
  });

  static constexpr auto get_2 = mem_fn([](auto self) -> lf::task<int> {
    co_await self->run(*self, 10);
    co_return self->m_private;
  });

private:
  static constexpr auto run = mem_fn([](auto self, int n) -> lf::task<> {
    if (n < 0) {
      co_return;
    }
    co_await lf::call(self->run)(*self, n - 1);
    co_await join;
  });

  int m_private = 99;
};

inline constexpr auto mem_from_coro = fn([](auto self) -> lf::task<int> {
  access_test a;
  int r;
  co_await lf::call(r, access_test::get)(a);
  co_await join;
  co_return r;
});

#if LIBFORK_PROPAGATE_EXCEPTIONS

struct deep : std::exception {};

inline constexpr auto deep_except = fn([](auto self, int n) -> lf::task<> {
  if (n > 0) {
    co_await lf::call(self)(n - 1);

    try {
      co_await lf::join;
      FAIL("Should not reach here");
    } catch (deep const &) {
      throw;
    }
  }
  throw deep{};
});

inline constexpr auto deep_except_2 = fn([](auto self, int n) -> lf::task<> {
  if (n > 0) {

    try {
      co_await self(n - 1);
      FAIL("Should not reach here");
    } catch (deep const &) {
      throw;
    }
  }
  throw deep{};
});

#endif

inline constexpr auto noop = fn([](auto self) -> lf::task<> { co_return; });

// In some implementations, this could cause a stack overflow if symmetric transfer is not used.
inline constexpr auto sym_stack_overflow_1 = fn([](auto self) -> lf::task<int> {
  for (int i = 0; i < 100'000'000; ++i) {
    co_await lf::fork(noop)();
    co_await lf::call(noop)();
    co_await join;
  }

  co_return 1;
});

template <scheduler S>
void test(S &schedule) {
  SECTION("stack-overflow") {
    REQUIRE(sync_wait(schedule, sym_stack_overflow_1));
  }

  SECTION("Fibonacci") {
    for (int i = 0; i < 25; ++i) {
      REQUIRE(fib(i) == sync_wait(schedule, r_fib, i));
    }
  }

  SECTION("Fibonacci inline") {
    for (int i = 0; i < 25; ++i) {
      LIBFORK_LOG("i={}", i);
      REQUIRE(fib(i) == sync_wait(schedule, r_fib_2, i));
    }
  }
  SECTION("Void Fibonacci") {
    int res;

    int i = 15;

    // for (int i = 15; i < 16; ++i) {
    sync_wait(schedule, v_fib, res, i);
    REQUIRE(fib(i) == res);
    // }
  }
  SECTION("member function") {
    access_test a;
    REQUIRE(99 == sync_wait(schedule, access_test::get, a));
    REQUIRE(99 == sync_wait(schedule, access_test::get_2, a));
    REQUIRE(99 == sync_wait(schedule, mem_from_coro));
  }

#if LIBFORK_PROPAGATE_EXCEPTIONS
  SECTION("exception propagate") {
    REQUIRE_THROWS_AS(sync_wait(schedule, deep_except, 10), deep);
  }

  SECTION("exception propagate from invoked") {
    REQUIRE_THROWS_AS(sync_wait(schedule, deep_except_2, 10), deep);
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