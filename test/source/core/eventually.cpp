/*
 * Copyright (c) Conor Williams, Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <libfork/core/async.hpp>
#include <libfork/core/utility.hpp>
#include <libfork/schedule/lazy_pool.hpp>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"
#include "libfork/schedule.hpp"

#include "libfork/core/eventually.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

TEST_CASE("Eventually, trivial types", "[eventually]") {
  eventually<int> a;
  a = 2;

  eventually<float> b;
  b.emplace(9);

  REQUIRE(*a == 2);
  REQUIRE(*b == 9);
}

TEST_CASE("Eventually, non-trivial types", "[eventually]") {
  eventually<std::vector<float>> a;

  a = std::vector<float>{1, 2, 3};

  eventually<std::vector<float>> b;

  std::vector<float> c = *std::move(a);

  b.emplace(c.begin(), c.end());

  REQUIRE(c == *b);
}

// An immovable, non-default-constructible type
struct difficult : lf::impl::immovable<difficult> {
  difficult(int) {}
  difficult(int, int) {}
};

void use_difficult(difficult const &) {
  // Do something...
}

inline constexpr lf::async make_difficult = [](auto, bool opt) -> lf::task<difficult> {
  if (opt) {
    co_return 34;
  } else {
    co_return lf::in_place{1, 2};
  }
};

inline constexpr lf::async eventually_example = [](auto) -> lf::task<> {
  // Delay construction:
  lf::eventually<difficult> a, b;

  // Make two difficult objects (in parallel):
  co_await lf::fork(a, make_difficult)(true);
  co_await lf::call(b, make_difficult)(false);

  // Wait for both to complete:
  co_await lf::join;

  // Now we can access the values:
  use_difficult(*a);
  use_difficult(*b);
};

TEST_CASE("Eventually, example", "[eventually]") {

  lf::lazy_pool pool{1};

  lf::sync_wait(pool, eventually_example);
}

// NOLINTEND