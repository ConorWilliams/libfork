// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"
#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"
#include "libfork/schedule/inline.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

using namespace lf::detail;

inline constexpr auto count = [](auto count, int &var) -> task<void> {
  if (var > 0) {
    --var;
  }
  co_return;
};

#include <iostream>

TEST_CASE("basic counting", "[inline_scheduler]") {

  //
  // inline_scheduler::context_type ctx;

  root_result<void> block;

  struct Head : basic_first_arg<root_result<void>, tag::root, decltype(count)> {
    using context_type = inline_scheduler::context_type;
  };

  int x = 10;

  count(Head{basic_first_arg<root_result<void>, tag::root, decltype(count)>{block}}, x);

  auto *root = tls::asp;

  REQUIRE(root);

  tls::asp = tls::sbuf.pop()->sentinel();

  REQUIRE(x == 10);
  root->get_coro().resume();
  REQUIRE(x == 9);

  // count(x);

  // std::cout << "------------------------" << x << std::endl;

  // REQUIRE(x == 8);

  //
}

// NOLINTEND