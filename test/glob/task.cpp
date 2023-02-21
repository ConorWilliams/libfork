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

#include "libfork/schedule/inline.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

using namespace lf;

template <typename T>
using task = basic_task<T, inline_context>;

template <typename T>
using future = basic_future<T, inline_context>;

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

  inline_context context{};

  REQUIRE(context.empty());

  DEBUG_TRACKER("testing noop");

  for (int i = 0; i < 100; ++i) {
    auto [fut, task] = noop().make_promise();
    task.resume_root(context);
    REQUIRE(context.empty());
  }

  DEBUG_TRACKER("testing set");

  for (int i = 0; i < 100; ++i) {
    int x = 0;
    DEBUG_TRACKER("A");
    auto [fut, task] = set(x, i).make_promise();
    DEBUG_TRACKER("B");
    task.resume_root(context);
    DEBUG_TRACKER("C");
    REQUIRE(x == i);
    DEBUG_TRACKER("D");
    REQUIRE(context.empty());
  }

  DEBUG_TRACKER("testing fwd");

  for (int i = 0; i < 100; ++i) {
    auto [fut, task] = fwd(i).make_promise();
    task.resume_root(context);
    REQUIRE(*fut == i);
    REQUIRE(context.empty());
  }

  DEBUG_TRACKER("testing fwd&");

  for (int i = 0; i < 100; ++i) {
    //
    int tmp = i;

    auto [fut, task] = fwd<int&>(tmp).make_promise();
    task.resume_root(context);

    REQUIRE(context.empty());

    tmp += 1;

    REQUIRE(*fut == i + 1);

    *fut -= 1;

    REQUIRE(tmp == i);
  }

  DEBUG_TRACKER("testing fwd {std::string}");

  for (int i = 0; i < 100; ++i) {
    auto [fut, task] = fwd(std::to_string(i)).make_promise();
    task.resume_root(context);
    REQUIRE(*fut == std::to_string(i));
    REQUIRE(context.empty());
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
  inline_context context{};

  for (int j = 0; j < 100; ++j) {
    for (int i = 0; i < 10; ++i) {
      auto [fut, task] = fib_task(i).make_promise();
      task.resume_root(context);
      REQUIRE(*fut == fib(i));
      REQUIRE(context.empty());
    }
  }
}

TEST_CASE("Fibonacci - void", "[basic_task]") {
  //
  inline_context context{};

  for (int j = 0; j < 100; ++j) {
    for (int i = 0; i < 10; ++i) {
      int x = 0;
      DEBUG_TRACKER("iter");
      auto [fut, task] = fib_task_void(x, i).make_promise();
      task.resume_root(context);
      REQUIRE(x == fib(i));
      REQUIRE(context.empty());
    }
  }
}

// In some implementations, this could cause a stack overflow.
static task<int> stack_overflow() {
  for (int i = 0; i < 100'000; ++i) {
    co_await noop().fork();
    co_await fwd(i).fork();

    co_await noop();
    co_await fwd(i);

    co_await join();
  }
}

TEST_CASE("Stack overflow", "[basic_task]") {
  //
  inline_context context{};
  auto [fut, task] = stack_overflow().make_promise();
  task.resume_root(context);
  REQUIRE(context.empty());
}

// NOLINTEND