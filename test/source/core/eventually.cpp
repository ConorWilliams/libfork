// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

TEST_CASE("Eventually, trivial types", "[eventually]") {
  eventually<int> a;
  a = 2;

  eventually<float> b;
  b = 9;

  REQUIRE(*a == 2);
  REQUIRE(*b == 9);
}

TEST_CASE("Eventually, non-trivial types", "[eventually]") {
  eventually<std::vector<float>> a;

  a = std::vector<float>{1, 2, 3};

  eventually<std::vector<float>> b;

  std::vector<float> c = *std::move(a);

  b = std::vector<float>(c.begin(), c.end());

  REQUIRE(c == *b);
}

// NOLINTEND