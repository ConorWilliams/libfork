// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

#include "libfork/inline.hpp"
#include "libfork/task.hpp"
#include "libfork/utility.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

task<int, inline_context> fib(int x) {
  //
  if (x < 2) {
    co_return x;
  }

  future<int> a, b;

  co_await fork(a, fib(x - 1));
  co_await just(b, fib(x - 2));

  co_await join;

  co_return a + b;
}

/**
 * Once you .resume() a task it will run until it is done or you loose a sync().
 *
 * If you loose a sync() then some-else has promised to finish it.
 *
 */

TEST_CASE("Basic task manipulation", "[task]") {
  int count = 0;

  auto t = fib(2);

  FORK_LOG("that was the root one");

  inline_context context{};

  context.push(t.m_promise);

  future<int> result;

  t.m_promise->set_result_ptr(result);
  t.m_promise->m_stack = &context;

  context.pop()->m_this.resume();

  REQUIRE(false);
}

// NOLINTEND