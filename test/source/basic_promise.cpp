// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"
#include "libfork/schedule/inline.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

inline constexpr async count = [](auto count, int &var) -> task<void> {
  if (var > 0) {
    --var;
  }
  co_return;
};

TEST_CASE("basic counting", "[inline_scheduler]") {
  inline_scheduler::context_type ctx;

  int x = 10;

  packet p = count(x);

  //
}

// NOLINTEND