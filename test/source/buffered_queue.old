// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

#include "libfork/schedule/busy.hpp"

// NOLINTBEGIN

using namespace lf::detail;

TEST_CASE("basic_operations", "[buffered]") {

  buffered_queue<int, 4> queue;

  SECTION("Within buffer") {

    REQUIRE(queue.empty());
    REQUIRE(!queue.pop());
    REQUIRE(!queue.steal());

    for (int i = 0; i < 4; ++i) {
      queue.push(i);
    }

    REQUIRE(!queue.empty());
    REQUIRE(!queue.steal());

    for (int i = 3; i >= 0; --i) {
      auto item = queue.pop();
      REQUIRE(item);
      REQUIRE(*item == i);
    }

    REQUIRE(queue.empty());
    REQUIRE(!queue.steal());
  }

  SECTION("Beyond buffer") {

    REQUIRE(queue.empty());
    REQUIRE(!queue.pop());
    REQUIRE(!queue.steal());

    for (int i = 0; i < 24; ++i) {
      queue.push(i);
    }

    REQUIRE(!queue.empty());

    auto first = queue.steal();

    REQUIRE(first);
    REQUIRE(*first == 0);

    for (int i = 23; i > 0; --i) {
      auto item = queue.pop();
      REQUIRE(item);
      REQUIRE(*item == i);
    }

    REQUIRE(queue.empty());
  }
}

// NOLINTEND