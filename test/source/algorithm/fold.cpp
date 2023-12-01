// // Copyright © Conor Williams <conorwilliams@outlook.com>

// // SPDX-License-Identifier: MPL-2.0

// // This Source Code Form is subject to the terms of the Mozilla Public
// // License, v. 2.0. If a copy of the MPL was not distributed with this
// // file, You can obtain one at https://mozilla.org/MPL/2.0/.

// // #define NDEBUG
// // #define LF_COROUTINE_OFFSET 2 * sizeof(void *)

// #include <vector>

// #include <catch2/benchmark/catch_benchmark.hpp>
// #include <catch2/catch_template_test_macros.hpp>
// #include <catch2/catch_test_macros.hpp>

// #include "libfork/schedule.hpp"

// #include "libfork/algorithm/fold.hpp"

// // NOLINTBEGIN No linting in tests

// using namespace lf;

// TEMPLATE_TEST_CASE("fold", "[algorithm][template]", std::vector<int>) {

//   TestType v;

//   constexpr int n = 10'000;

//   for (auto i = 1; i <= n; i++) {
//     v.push_back(i);
//   }

//   constexpr int correct = n * (n + 1) / 2;

//   lf::lazy_pool pool{};

//   auto times_2 = [](auto x) {
//     return 2 * x;
//   };

//   // --------------- First regular function --------------- //

//   {
//     auto fun = std::plus<>{};

//     //   Check grain = 1 case:
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, fun) == correct);

//     // Check grain > 1 and n % grain == 0 case:
//     REQUIRE(v.size() % 100 == 0);
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 100, fun) == correct);

//     // Check grain > 1 and n % grain != 0 case:
//     REQUIRE(v.size() % 300 != 0);
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 300, fun) == correct);

//     // Check grain > size case:
//     REQUIRE(v.size() < 20'000);
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 20'000, fun) == correct);

//     // ----- With projection ---- //

//     REQUIRE(lf::sync_wait(pool, lf::fold, v, fun, times_2) == 2 * correct);

//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 300, fun, times_2) == 2 * correct);
//   }

//   // --------------- Now async + mixed--------------- //

//   {

//     async fun = [](auto, auto a, auto b) -> task<long> {
//       co_return a + b;
//     };

//     //   Check grain = 1 case:
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, fun) == correct);

//     // Check grain > 1 and n % grain == 0 case:
//     REQUIRE(v.size() % 100 == 0);
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 100, fun) == correct);

//     // Check grain > 1 and n % grain != 0 case:
//     REQUIRE(v.size() % 300 != 0);
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 300, fun) == correct);

//     // Check grain > size case:
//     REQUIRE(v.size() < 20'000);
//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 20'000, fun) == correct);

//     // ----- With projection ---- //

//     REQUIRE(lf::sync_wait(pool, lf::fold, v, fun, times_2) == 2 * correct);

//     REQUIRE(lf::sync_wait(pool, lf::fold, v, 300, fun, times_2) == 2 * correct);
//   }

//   // ----------- Now with small inputs ----------- //

//   REQUIRE(lf::sync_wait(pool, fold, std::span(v.data(), 4), std::plus<>{}) == 4 * (4 + 1) / 2);
//   REQUIRE(lf::sync_wait(pool, fold, std::span(v.data(), 3), std::plus<>{}) == 3 * (3 + 1) / 2);
//   REQUIRE(lf::sync_wait(pool, fold, std::span(v.data(), 2), std::plus<>{}) == 2 * (2 + 1) / 2);
//   REQUIRE(lf::sync_wait(pool, fold, std::span(v.data(), 1), std::plus<>{}) == 1 * (1 + 1) / 2);
//   // REQUIRE(!lf::sync_wait(pool, fold, std::span(v.data(), 0), std::plus<>{}));

//   // ------------------ with large n ------------------ //
// }