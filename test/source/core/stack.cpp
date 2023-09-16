// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #define NDEBUG
// #define LF_COROUTINE_OFFSET 2 * sizeof(void *)

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/core/stack.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

using namespace lf::impl;

struct root_task {
  struct promise_type : promise_alloc_heap {

    promise_type() : promise_alloc_heap{stdx::coroutine_handle<promise_type>::from_promise(*this)} {}

    auto get_return_object() noexcept -> root_task { return {this}; }
    auto initial_suspend() noexcept -> stdx::suspend_always { return {}; }
    auto final_suspend() noexcept {
      struct implode : stdx::suspend_always {
        auto await_suspend(stdx::coroutine_handle<promise_type> h) noexcept { h.destroy(); }
      };
      return implode{};
    }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };

  frame_block *frame;
};

auto root(auto fun) -> root_task { co_return fun(); }

TEST_CASE("Root task", "[virtual_stack]") {

  int x = 0;

  root_task t = root([&] { ++x; });

  REQUIRE(x == 0);

  t.frame->coro().resume();

  REQUIRE(x == 1);
}

struct non_root_task {
  struct promise_type : promise_alloc_stack {

    promise_type() : promise_alloc_stack{stdx::coroutine_handle<promise_type>::from_promise(*this)} {}

    auto get_return_object() noexcept -> non_root_task { return {this}; }
    auto initial_suspend() noexcept -> stdx::suspend_always { return {}; }
    auto final_suspend() noexcept {
      struct implode : stdx::suspend_always {
        auto await_suspend(stdx::coroutine_handle<promise_type> h) noexcept { h.destroy(); }
      };
      return implode{};
    }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };

  frame_block *frame;
};

auto fib(int &res, int n) -> non_root_task {
  if (n <= 1) {
    res = n;
  } else {
    int x, y;

    fib(x, n - 1).frame->coro().resume();

    fib(y, n - 2).frame->coro().resume();

    res = x + y;
  }

  co_return;
}

void inline_fib(int &res, int n) {
  if (n <= 1) {
    res = n;
  } else {
    int a, b;

    inline_fib(a, n - 1);
    inline_fib(b, n - 2);

    res = a + b;
  }
}

TEST_CASE("fib on stack", "[virtual_stack]") {

  root([&] {
    //
    auto *s = new async_stack{};

    tls::asp = stack_as_bytes(s);

    volatile int p = 20;

    int y = 0;

    BENCHMARK("Fibonacci " + std::to_string(p) + " function") {
      inline_fib(y, p);
      return y;
    };

    int x = 1;

    BENCHMARK("Fibonacci " + std::to_string(p) + " coroutine") {
      fib(x, p).frame->coro().resume();
      return x;
    };

    REQUIRE(x == y);

    auto *f = bytes_to_stack(tls::asp);

    REQUIRE(s == f);

    delete s;
  })
      .frame->coro()
      .resume();
}

// NOLINTEND