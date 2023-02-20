// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <numeric>
#include <span>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define NLOG
// #define NDEBUG

// NOLINTBEGIN No need to check the tests for style.

#include "libfork/allocator.hpp"
#include "libfork/basic_task.hpp"
#include "libfork/inline.hpp"
#include "libfork/utility.hpp"

using namespace lf;

template <typename T>
using task = basic_task<T, inline_context>;

template <typename T>
using future = basic_future<T, inline_context>;

task<void> noop() {
  co_return;
}

template <typename T>
task<void> set(T& out, T in) {
  out = std::move(in);
  co_return;
}

template <typename T>
task<T> fwd(T x) {
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
    auto [fut, task] = set(x, i).make_promise();
    task.resume_root(context);
    REQUIRE(x == i);
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
int fib(int n) {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

// Fibonacci using tasks
task<int> fib_task(int n) {
  if (n <= 1) {
    co_return n;
  }

  auto a = co_await fib_task(n - 1).fork();
  auto b = co_await fib_task(n - 2).just();

  co_await join();

  co_return *a + b;
}

// Fibonacci using void tasks
task<void> fib_task_void(int& out, int n) {
  if (n <= 1) {
    out = n;
    co_return;
  }

  int a, b;

  co_await fib_task_void(a, n - 1).fork();
  co_await fib_task_void(b, n - 2).just();

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
      auto [fut, task] = fib_task_void(x, i).make_promise();
      task.resume_root(context);
      REQUIRE(x == fib(i));
      REQUIRE(context.empty());
    }
  }
}

// NOLINTEND