// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>  // ssize?

#include <catch2/catch_test_macros.hpp>

#include "libfork/queue.hpp"

TEST_CASE("Single thread as stack", "[queue]") {
  lf::queue<int> queue;

  REQUIRE(queue.empty());

  for (int i = 0; i < 10; ++i) {
    queue.push(i);
    REQUIRE(queue.ssize() == i + 1);
  }

  for (int i = 9; i >= 0; --i) {
    auto item = queue.pop();
    REQUIRE(item);
    REQUIRE(*item == i);
  }

  REQUIRE(queue.empty());
}
