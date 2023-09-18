// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

#include "libfork/schedule/ring_buffer.hpp"

// NOLINTBEGIN

using namespace lf;

using namespace lf::ext;

TEST_CASE("basic_operations", "[buffered]") {

  ring_buffer<int, 4> queue;

  REQUIRE(queue.empty());
  REQUIRE(!queue.pop());

  for (int i = 0; i < 4; ++i) {
    REQUIRE(queue.push(i));
  }

  REQUIRE(queue.full());
  REQUIRE(!queue.push(4));

  REQUIRE(!queue.empty());

  for (int i = 3; i >= 0; --i) {
    auto item = queue.pop();
    REQUIRE(item);
    REQUIRE(*item == i);
  }

  REQUIRE(queue.empty());
}

// NOLINTEND