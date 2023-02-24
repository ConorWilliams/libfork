// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN No need to check the tests for style.

#include "libfork/schedule/immediate.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

using namespace lf;

template <typename T>
using task = basic_task<T, immediate::context>;

template <typename T>
using future = basic_future<T, immediate::context>;

static task<void> noop() {
  co_return;
}

template <typename T>
static task<void> set(T& out, T in) {
  out = std::move(in);
  co_return;
}

template <typename T>
static task<T> fwd(T x) {
  co_return x;
}

TEST_CASE("Trivial tasks", "[basic_task]") {
  //

  immediate sch{};

  REQUIRE(sch.empty());

  DEBUG_TRACKER("testing noop");

  for (int i = 0; i < 100; ++i) {
    sch.schedule(noop());
    REQUIRE(sch.empty());
  }

  DEBUG_TRACKER("testing set");

  for (int i = 0; i < 100; ++i) {
    int x = 0;
    sch.schedule(set(x, i));
    REQUIRE(x == i);
    REQUIRE(sch.empty());
  }

  DEBUG_TRACKER("testing fwd");

  for (int i = 0; i < 100; ++i) {
    REQUIRE(sch.schedule(fwd(i)) == i);
    REQUIRE(sch.empty());
  }

  DEBUG_TRACKER("testing fwd&");

  for (int i = 0; i < 100; ++i) {
    //
    int tmp = i;

    int& fut = sch.schedule(fwd<int&>(tmp));

    REQUIRE(sch.empty());

    tmp += 1;

    REQUIRE(fut == i + 1);

    fut -= 1;

    REQUIRE(tmp == i);
  }

  DEBUG_TRACKER("testing fwd {std::string}");

  for (int i = 0; i < 100; ++i) {
    REQUIRE(sch.schedule(fwd(std::to_string(i))) == std::to_string(i));
    REQUIRE(sch.empty());
  }
}

task<void> double_void() {
  co_return co_await noop();
}

TEST_CASE("double void tasks", "[basic_task]") {
  immediate sch{};

  REQUIRE(sch.empty());

  DEBUG_TRACKER("testing double_void");

  for (int i = 0; i < 100; ++i) {
    sch.schedule(double_void());
    REQUIRE(sch.empty());
  }
}

// Fibonacci using recursion
static int fib(int n) {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

// Fibonacci using tasks
static task<int> fib_task(int n) {
  if (n <= 1) {
    co_return n;
  }

  auto a = co_await fib_task(n - 1).fork();
  auto b = co_await fib_task(n - 2);

  co_await join();

  co_return *a + b;
}

// Fibonacci using void tasks
static task<void> fib_task_void(int& out, int n) {
  if (n <= 1) {
    out = n;
    co_return;
  }

  int a, b;

  co_await fib_task_void(a, n - 1).fork();
  co_await fib_task_void(b, n - 2);

  co_await join();

  out = a + b;
}

TEST_CASE("Fibonacci - int", "[basic_task]") {
  //
  immediate sch{};

  for (int j = 0; j < 100; ++j) {
    for (int i = 0; i < 10; ++i) {
      REQUIRE(sch.schedule(fib_task(i)) == fib(i));
      REQUIRE(sch.empty());
    }
  }
}

TEST_CASE("Fibonacci - void", "[basic_task]") {
  //
  immediate sch{};

  for (int j = 0; j < 100; ++j) {
    for (int i = 0; i < 10; ++i) {
      int x = 0;
      sch.schedule(fib_task_void(x, i));
      REQUIRE(x == fib(i));
      REQUIRE(sch.empty());
    }
  }
}

// In some implementations, this could cause a stack overflow.
static task<int> stack_overflow() {
  for (int i = 0; i < 500'000; ++i) {
    DEBUG_TRACKER("iter\n");

    co_await noop().fork();
    co_await fwd(i).fork();

    co_await noop();
    co_await fwd(i);

    co_await join();
  }
}

// Marked as !mayfail && !benchmark due to GCC not being able to properly optimize
// symmetric transfer into tail calls.
TEST_CASE("Stack overflow", "[basic_task][!benchmark][!mayfail]") {
  //
  immediate sch{};
  sch.schedule(stack_overflow());
  REQUIRE(sch.empty());
}

// NOLINTEND