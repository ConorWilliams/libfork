// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <semaphore>
#include <stack>
#include <thread>
#include <type_traits>
#include <utility>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

// #define NDEBUG

// #define LF_DEFAULT_LOGGING

// #define LF_ASSERT(x)

// #define LF_PROPAGATE_EXCEPTIONS
// #undef LF_LOG

// #define LF_DEFAULT_LOGGING

#include "libfork/core.hpp"

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/schedule/lazy_pool.hpp"
#include "libfork/schedule/unit_pool.hpp"

#include "libfork/algorithm/lift.hpp"

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

inline constexpr auto noop = [](auto) -> task<> {
  LF_LOG("nooop");
  co_return;
};

} // namespace

//  unit_pool, debug_pool, busy_pool, lazy_pool

TEMPLATE_TEST_CASE("Construct destruct launch", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  for (int i = 0; i < 100; ++i) {
    auto schedule = make_scheduler<TestType>();
  }

  for (int i = 0; i < 100; ++i) {

    auto schedule = make_scheduler<TestType>();

    for (int j = 0; j < 100; ++j) {
      sync_wait(schedule, noop);
    }
  }
}

// ------------------------ stack overflow ------------------------ //

namespace {

// In some implementations, this could cause a stack overflow if symmetric transfer is not used.
inline constexpr auto sym_stack_overflow_1 = [](auto, int n) -> lf::task<int> {
  for (int i = 0; i < n; ++i) {
    co_await lf::call(noop)();
  }
  co_return 1;
};

} // namespace

/**
 * This test is known to fail with gcc in debug due to no tail call optimization in debug builds,
 * if this test fails then probably others will as well.
 */
TEST_CASE("Stack overflow - sym-transfer", "[core]") {
  SECTION("iter = 1") { REQUIRE(sync_wait(unit_pool{}, sym_stack_overflow_1, 1)); }
  SECTION("iter = 100") { REQUIRE(sync_wait(unit_pool{}, sym_stack_overflow_1, 100)); }
  SECTION("iter = 10'000") { REQUIRE(sync_wait(unit_pool{}, sym_stack_overflow_1, 10'000)); }
  SECTION("iter = 100'000") { REQUIRE(sync_wait(unit_pool{}, sym_stack_overflow_1, 100'000)); }
  SECTION("iter = 1'000'000") { REQUIRE(sync_wait(unit_pool{}, sym_stack_overflow_1, 1'000'000)); }
  SECTION("iter = 100'000'000") { REQUIRE(sync_wait(unit_pool{}, sym_stack_overflow_1, 100'000'000)); }
}

// // ------------------------ Fibonacci ------------------------ //

namespace {

int fib(int n) {
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

  int a, b;

  co_await lf::fork(&a, fib)(n - 1);
  co_await lf::call(&b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

} // namespace

TEMPLATE_TEST_CASE("Fibonacci - returning", "[core][template]", unit_pool, busy_pool, lazy_pool) {
  for (int j = 0; j < 100; ++j) {
    {
      auto schedule = make_scheduler<TestType>();

      for (int i = 1; i < 20; ++i) {
        REQUIRE(fib(i) == sync_wait(schedule, r_fib, i));
      }
    }
  }
}

namespace {

inline constexpr auto v_fib = [](auto fib, int &ret, int n) -> lf::task<void> {
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

  int c;

  co_await lf::call(fib)(c, n - 2);

  LF_ASSERT(b == c);

  ret = a + b;
};

}

TEMPLATE_TEST_CASE("Fibonacci - void", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  auto schedule = make_scheduler<TestType>();

  for (int i = 0; i < 15; ++i) {
    int res;
    sync_wait(schedule, v_fib, res, i);
    REQUIRE(fib(i) == res);
  }
}

// ------------------------ differing return types ------------------------ //

namespace {

inline constexpr auto v_fib_ignore = [](auto fib, int &ret, int n) -> lf::task<int> {
  //
  if (n < 2) {
    ret = n;
    co_return n;
  }

  int a, b;

  std::optional<int> c;

  for (int i = 0; i < 2; i++) {
    co_await lf::fork(fib)(a, n - 1);     // Test explicit ignore
    co_await lf::call(&c, fib)(b, n - 2); // Test bind to a different type.
    co_await lf::join;

    LF_ASSERT(c);
    LF_ASSERT(b == c);
  }

  ret = a + b;

  co_return a + b;
};

}

TEMPLATE_TEST_CASE("Fibonacci - ignored", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  auto schedule = make_scheduler<TestType>();

  for (int i = 3; i < 15; ++i) {
    int res;
    int x = sync_wait(schedule, v_fib_ignore, res, i);
    REQUIRE(fib(i) == res);
    REQUIRE(res == x);
    LF_LOG("post");
  }
}

// ------------------------ References and member functions ------------------------ //

namespace {

class ref_test {
 public:
  static constexpr auto get = [](auto, auto &&self) -> lf::task<int &> {
    //
    // auto &also_prov =

    lf::eventually<int &> also_prov;

    co_await lf::call(&also_prov, self.get_2)(self);

    LF_ASSERT(&(*also_prov) == &self.m_private);

    co_return self.m_private;
  };

  int m_private = 99;

 private:
  static constexpr auto get_2 = [](auto, auto &&self) -> lf::task<int &> {
    co_return self.m_private;
  };
};

} // namespace

TEMPLATE_TEST_CASE("Reference test", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  LF_LOG("pre-init");

  auto schedule = make_scheduler<TestType>();

  ref_test a;

  int &r = sync_wait(schedule, ref_test::get, a);

  REQUIRE(r == 99);

  r = 100;

  REQUIRE(a.m_private == 100);
}

// ------------------------- CO ALLOC ------------------------- //

namespace {

inline constexpr auto co_fib = [](auto co_fib, int n) -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  auto [a] = co_await lf::co_new<int>(1);
  auto [b] = co_await lf::co_new<int>(1);

  co_await lf::fork(&a[0], co_fib)(n - 1);
  co_await lf::call(&b[0], co_fib)(n - 2);

  co_await lf::join;

  co_return a[0] + b[0];
};

} // namespace

TEMPLATE_TEST_CASE("Fibonacci - co_alloc", "[core][template]", unit_pool, busy_pool, lazy_pool) {

  for (int j = 0; j < 100; ++j) {
    {
      auto schedule = make_scheduler<TestType>();

      for (int i = 1; i < 20; ++i) {
        REQUIRE(fib(i) == sync_wait(schedule, co_fib, i));
      }
    }
  }
}

// NOLINTEND