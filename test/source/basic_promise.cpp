// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define LF_LOGGING
#define NDEBUG

#include "libfork/core.hpp"
#include "libfork/core/call.hpp"
#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"
#include "libfork/schedule/inline.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

using namespace lf::detail;

inline constexpr auto count = [](auto count, int &var) static -> task<void, "count"> {
  if (var > 0) {
    --var;
    co_await lf::fork[count](var);
    co_await lf::join;
  }
  co_return;
};

TEST_CASE("basic counting", "[inline_scheduler]") {

  root_result<void> block;

  using C = inline_vec::context_type;

  using base = basic_first_arg<root_result<void>, tag::root, decltype(count)>;

  struct Head : base {
    using context_type = C;
  };

  int x = 10;

  static_assert(first_arg<Head>);

  auto arg = Head{base{block}};

  auto root = count(std::move(arg), x);

  C ctx;

  worker_init(&ctx);

  REQUIRE(x == 10);

  ctx.submit(root.frame());

  block.semaphore.acquire();

  LF_LOG("x = {}", x);

  REQUIRE(x == 0);

  worker_finalize(&ctx);
}

__attribute__((noinline)) void inline_fiber(int &res, int n) {
  if (n <= 1) {
    res = n;
  } else {
    int a, b;

    inline_fiber(a, n - 1);
    inline_fiber(b, n - 2);

    res = a + b;
  }
}

inline constexpr auto fib = [](auto fib, int n) static -> task<int> {
  //
  if (n <= 1) {
    co_return n;
  }

  int a;
  int b;

  co_await lf::fork[a, fib](n - 1);
  co_await lf::call[b, fib](n - 2);

  co_await lf::join;

  co_return a + b;
};

TEST_CASE("fib", "[promise]") {
  //

  using C = inline_scheduler::context_type;

  C ctx;

  worker_init(&ctx);

  using base = basic_first_arg<root_result<int>, tag::root, decltype(fib)>;

  struct Head : base {
    using context_type = C;
  };

  volatile int in = 30;

  int x;
  int y;

  BENCHMARK("inline") {
    inline_fiber(y, in);
    return y;
  };

  BENCHMARK("coroutine") {
    root_result<int> block;
    auto root = fib(Head{base{block}}, int(in));
    ctx.submit(root.frame());

    x = *std::move(block);

    return x;
  };

  REQUIRE(x == y);

  worker_finalize(&ctx);
}

// NOLINTEND