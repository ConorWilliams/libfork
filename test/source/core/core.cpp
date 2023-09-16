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

// #define LF_ASSERT(x)

// #define LF_PROPAGATE_EXCEPTIONS
// #undef LF_LOG
// #define LF_LOGGING

#include "libfork/core.hpp"
#include "libfork/schedule/inline.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

// ------------------------ Construct destruct ------------------------ //

inline constexpr async noop = [](auto) -> task<> {
  LF_LOG("nooop");
  co_return;
};

// TEMPLATE_TEST_CASE("Construct destruct launch", "[libfork][template]", inline_scheduler) {

//   for (int i = 0; i < 1000; ++i) {
//     TestType tmp{};
//   }

//   for (int i = 0; i < 100; ++i) {

//     TestType schedule{};

//     for (int j = 0; j < 100; ++j) {
//       sync_wait(schedule, noop);
//     }
//   }
// }

// ------------------------ stack overflow ------------------------ //

// In some implementations, this could cause a stack overflow if symmetric transfer is not used.
inline constexpr async sym_stack_overflow_1 = [](auto) -> lf::task<int> {
  for (int i = 0; i < 100'000'000; ++i) {
    co_await lf::fork[noop]();
    co_await lf::call(noop)();
  }

  co_await lf::join;

  co_return 1;
};

TEMPLATE_TEST_CASE("Stack overflow - sym-transfer", "[libfork][template]", inline_scheduler) {
  REQUIRE(sync_wait(TestType{}, sym_stack_overflow_1));
}

// // ------------------------ Fibonacci ------------------------ //

int fib(int n) {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

inline constexpr async r_fib = [](auto fib, int n) -> lf::task<int> {
  //

  REQUIRE(fib.context().max_threads() >= 0);

  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, fib)(n - 1);
  co_await lf::call(b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

TEMPLATE_TEST_CASE("Fibonacci - returning", "[libfork][template]", inline_scheduler) {

  TestType schedule{};

  for (int i = 0; i < 25; ++i) {
    REQUIRE(fib(i) == sync_wait(schedule, r_fib, i));
  }
}

inline constexpr async inline_fib = [](auto fib, int n) -> lf::task<int> {
  //

  if (n < 2) {
    co_return n;
  }

  int a = co_await fib(n - 1);
  int b = co_await fib(n - 2);

  co_return a + b;
};

TEMPLATE_TEST_CASE("Fibonacci - inline", "[libfork][template]", inline_scheduler) {

  TestType schedule{};

  for (int i = 0; i < 25; ++i) {
    REQUIRE(fib(i) == sync_wait(schedule, inline_fib, i));
  }
}

inline constexpr async v_fib = [](auto fib, int &ret, int n) -> lf::task<void> {
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
};

TEMPLATE_TEST_CASE("Fibonacci - void", "[libfork][template]", inline_scheduler) {

  TestType schedule{};

  for (int i = 0; i < 15; ++i) {
    int res;
    sync_wait(schedule, v_fib, res, i);
    REQUIRE(fib(i) == res);
  }
}

// ------------------------ differing return types ------------------------ //

inline constexpr async v_fib_ignore = [](auto fib, int &ret, int n) -> lf::task<int> {
  //
  if (n < 2) {
    ret = n;
    co_return n;
  }

  int a, b;

  std::optional<int> c;

  for (int i = 0; i < 2; i++) {
    co_await lf::fork(fib)(a, n - 1);    // Test explicit ignore
    co_await lf::call(c, fib)(b, n - 2); // Test bind to a different type.
    co_await lf::join;

    REQUIRE(c);
    REQUIRE(b == c);
  }

  ret = a + b;

  co_return a + b;
};

TEMPLATE_TEST_CASE("Fibonacci - ignored", "[libfork][template]", inline_scheduler) {

  TestType schedule{};

  for (int i = 0; i < 15; ++i) {
    int res;
    int x = sync_wait(schedule, v_fib_ignore, res, i);
    REQUIRE(fib(i) == res);
    REQUIRE(res == x);
  }
}

// ------------------------ References and member functions ------------------------ //

class ref_test {
public:
  static constexpr async get = [](auto, auto &&self) -> lf::task<int &> {
    //
    auto &also_prov = co_await self.get_2(self);

    REQUIRE(&also_prov == &self.m_private);

    co_return self.m_private;
  };

  int m_private = 99;

private:
  static constexpr async get_2 = [](auto, auto &&self) -> lf::task<int &> { co_return self.m_private; };
};

TEMPLATE_TEST_CASE("Reference test", "[libfork][template]", inline_scheduler) {

  TestType schedule{};

  ref_test a;

  int &r = sync_wait(schedule, ref_test::get, a);

  REQUIRE(r == 99);

  r = 100;

  REQUIRE(a.m_private == 100);
}

// // NOLINTEND