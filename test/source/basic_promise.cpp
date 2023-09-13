// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define LF_LOGGING

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

  using base = basic_first_arg<root_result<void>, tag::root, decltype(count)>;

  struct Head : base {
    using context_type = inline_scheduler::context_type;
  };

  int x = 10;

  static_assert(first_arg<Head>);

  auto arg = Head{base{block}};

  auto root = count(std::move(arg), x);

  inline_scheduler::context_type ctx;

  worker_init(&ctx);

  REQUIRE(x == 10);

  ctx.submit(root.frame);

  block.semaphore.acquire();

  LF_LOG("x = {}", x);

  REQUIRE(x == 0);

  worker_finalize(&ctx);
}

// NOLINTEND