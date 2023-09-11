// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #define NDEBUG

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

using namespace lf::detail;

struct root_task {
  struct promise_type : promise_alloc_heap {

    promise_type() : promise_alloc_heap{stdx::coroutine_handle<promise_type>::from_promise(*this)} {}

    auto get_return_object() noexcept -> root_task { return {}; }
    auto initial_suspend() noexcept -> stdx::suspend_always { return {}; }
    auto final_suspend() noexcept -> stdx::suspend_always { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };
};

auto root(auto fun) -> root_task { co_return fun(); }

TEST_CASE("Root task", "[virtual_stack]") {

  int x = 0;

  root([&] { ++x; });

  REQUIRE(x == 0);
  REQUIRE(tls::asp);

  tls::asp->get_coro().resume();
  tls::asp->get_coro().destroy();

  REQUIRE(x == 1);
}

struct non_root_task {
  struct promise_type : promise_alloc_stack {

    promise_type() : promise_alloc_stack{stdx::coroutine_handle<promise_type>::from_promise(*this)} {}

    auto get_return_object() noexcept -> non_root_task { return {}; }
    auto initial_suspend() noexcept -> stdx::suspend_always { return {}; }
    auto final_suspend() noexcept {
      struct implode : stdx::suspend_always {

        auto await_suspend(stdx::coroutine_handle<promise_type> h) noexcept { frame_block::pop_asp(); }
      };
      return implode{};
    }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };
};

auto fib(int &res, int n) -> non_root_task {
  if (n <= 1) {
    res = n;
  } else {
    int x, y;

    fib(x, n - 1);
    tls::asp->get_coro().resume();

    fib(y, n - 2);
    tls::asp->get_coro().resume();

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

    tls::asp = s->sentinel();

    volatile int p = 30;

    int y;

    BENCHMARK("Fibonacci " + std::to_string(p) + " function") {
      inline_fib(y, p);
      return y;
    };

    int x;

    BENCHMARK("Fibonacci " + std::to_string(p) + " coroutine") {
      fib(x, p);
      tls::asp->get_coro().resume();

      return x;
    };

    REQUIRE(x == y);

    auto *f = async_stack::unsafe_from_sentinel(tls::asp);

    REQUIRE(s == f);

    delete s;
  });

  auto root_block = tls::asp;

  root_block->get_coro().resume();
  root_block->get_coro().destroy();
}

// NOLINTEND