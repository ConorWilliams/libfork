// // Copyright © Conor Williams <conorwilliams@outlook.com>

// // SPDX-License-Identifier: MPL-2.0

// // This Source Code Form is subject to the terms of the Mozilla Public
// // License, v. 2.0. If a copy of the MPL was not distributed with this
// // file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #include <catch2/catch_template_test_macros.hpp>
// #include <catch2/catch_test_macros.hpp>

// #include "libfork/algorithm.hpp"

// #include "libfork/schedule/busy.hpp"
// #include "libfork/schedule/inline.hpp"

// // NOLINTBEGIN

// using namespace lf;

// TEMPLATE_TEST_CASE("for_each", "[algorithm]", inline_scheduler, busy_pool) {
//   //
//   // std::vector<int> v;

//   // for (int i = 0; i < 10000; ++i) {
//   //   v.push_back(i);
//   // }

//   // TestType schedule{};

//   // sync_wait(schedule, detail::for_each, std::ranges::begin(v), std::ranges::end(v), [](auto &elem) {
//   //   elem *= 2;
//   // });

//   // for (auto &&elem : v) {
//   //   REQUIRE(elem % 2 == 0);
//   // }
// }

// // NOLINTEND