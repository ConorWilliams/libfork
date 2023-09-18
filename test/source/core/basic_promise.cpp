// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define LF_LOGGING
// #define NDEBUG
// #define LF_LOGGING

#include "libfork/core.hpp"
#include "libfork/core/call.hpp"
#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"
#include "libfork/schedule/unit_pool.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

using namespace lf::impl;

inline constexpr auto count = [](auto count, int &var) -> task<void, "count"> {
  if (var > 0) {
    --var;
    co_await lf::fork(count)(var);
    co_await lf::join;
  }
  co_return;
};

TEST_CASE("basic counting", "[unit_pool]") {

  root_result<void> block;

  using C = unit_pool::context_type;

  using base = basic_first_arg<root_result<void>, tag::root, decltype(count)>;

  using Head = patched<C, base>;

  int x = 10;

  static_assert(first_arg<Head>);

  auto arg = Head{base{block}};

  auto root = count(std::move(arg), x);

  C ctx;

  worker_init(&ctx);

  REQUIRE(x == 10);

  frame_node link{root.frame()};

  ctx.submit(&link);

  block.semaphore.acquire();

  LF_LOG("x = {}", x);

  REQUIRE(x == 0);

  worker_finalize(&ctx);
}

LF_NOINLINE void inline_fiber(int &res, int n) {
  if (n <= 1) {
    res = n;
  } else {
    int a, b;

    inline_fiber(a, n - 1);
    inline_fiber(b, n - 2);

    res = a + b;
  }
}

inline constexpr auto fib = [](auto fib, int n) -> task<int> {
  //
  if (n <= 1) {
    co_return n;
  }

  int a;
  int b;

  co_await lf::fork(a, fib)(n - 1);
  co_await lf::call(b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

inline constexpr auto fib_call = [](auto fib_call, int n) -> task<int> {
  //
  if (n <= 1) {
    co_return n;
  }

  int a;
  int b;

  co_await lf::call(a, fib_call)(n - 1);
  co_await lf::call(b, fib_call)(n - 2);

  co_await lf::join;

  co_return a + b;
};

TEST_CASE("fib-bench", "[promise]") {
  //

  volatile int in = 30;

  int trivial = 0;

  inline_fiber(trivial, in);

  BENCHMARK("inline") {
    inline_fiber(trivial, in);
    return trivial;
  };

  using C = unit_pool::context_type;

  C ctx;

  worker_init(&ctx);

  {

    using head = patched<C, basic_first_arg<root_result<int>, tag::root, decltype(fib_call)>>;

    int x = -1;

    BENCHMARK("coroutine call") {
      root_result<int> block;
      frame_node root{fib_call(head{{block}}, int(in)).frame()};
      ctx.submit(&root);
      x = *std::move(block);
      return x;
    };

    REQUIRE(x == trivial);
  }

  {

    using head = patched<C, basic_first_arg<root_result<int>, tag::root, decltype(fib)>>;

    int x = -1;

    BENCHMARK("coroutine fork") {
      root_result<int> block;
      frame_node root{fib(head{{block}}, int(in)).frame()};
      ctx.submit(&root);
      x = *std::move(block);
      return x;
    };

    REQUIRE(x == trivial);
  }

  worker_finalize(&ctx);
}

// NOLINTEND