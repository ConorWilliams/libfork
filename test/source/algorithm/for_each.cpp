// // Copyright © Conor Williams <conorwilliams@outlook.com>

// // SPDX-License-Identifier: MPL-2.0

// // This Source Code Form is subject to the terms of the Mozilla Public
// // License, v. 2.0. If a copy of the MPL was not distributed with this
// // file, You can obtain one at https://mozilla.org/MPL/2.0/.

// // #define NDEBUG
// // #define LF_COROUTINE_OFFSET 2 * sizeof(void *)

// #include <list>
// #include <vector>

// #include <catch2/benchmark/catch_benchmark.hpp>
// #include <catch2/catch_template_test_macros.hpp>
// #include <catch2/catch_test_macros.hpp>

// #include "libfork/algorithm/for_each.hpp"
// #include "libfork/schedule.hpp"

// // NOLINTBEGIN No linting in tests

// using namespace lf;

// template <typename T>
// void check(T const &v, int k) {
//   for (int i = 0; auto &&elem : v) {
//     REQUIRE(elem == i++ + k);
//   }
// }

// TEMPLATE_TEST_CASE("for each", "[algorithm][template]", std::vector<int>) {

//   int count = 0;

//   TestType v;

//   for (auto i = 0; i < 10'000; i++) {
//     v.push_back(i);
//   }

//   check(v, count++);

//   lf::lazy_pool pool{};

//   auto address = [](int &i) -> int * {
//     return &i;
//   };

//   // --------------- First regular function --------------- //

//   {
//     auto fun = [](int &i) {
//       i++;
//     };

//     // Check grain = 1 case:
//     lf::sync_wait(pool, lf::for_each, v, fun);
//     check(v, count++);

//     // Check grain > 1 and n % grain == 0 case:

//     REQUIRE(v.size() % 100 == 0);
//     lf::sync_wait(pool, lf::for_each, v, 100, fun);
//     check(v, count++);

//     // Check grain > 1 and n % grain != 0 case:
//     REQUIRE(v.size() % 300 != 0);
//     lf::sync_wait(pool, lf::for_each, v, 300, fun);
//     check(v, count++);

//     // Check grain > size case:
//     REQUIRE(v.size() < 20'000);
//     lf::sync_wait(pool, lf::for_each, v, 20'000, fun);
//     check(v, count++);

//     // ----- With projection ---- //

//     auto inc = [](int *i) {
//       (*i)++;
//     };

//     lf::sync_wait(pool, lf::for_each, v, inc, address);
//     check(v, count++);

//     lf::sync_wait(pool, lf::for_each, v, 300, inc, address);
//     check(v, count++);
//   }

//   // --------------- Now async --------------- //

//   {
//     async fun = [](auto, int &i) -> task<> {
//       i++;
//       co_return;
//     };

//     // Check grain = 1 case:
//     lf::sync_wait(pool, lf::for_each, v, fun);
//     check(v, count++);

//     // Check grain > 1 and n % grain == 0 case:

//     REQUIRE(v.size() % 100 == 0);
//     lf::sync_wait(pool, lf::for_each, v, 100, fun);
//     check(v, count++);

//     // Check grain > 1 and n % grain != 0 case:
//     REQUIRE(v.size() % 300 != 0);
//     lf::sync_wait(pool, lf::for_each, v, 300, fun);
//     check(v, count++);

//     // Check grain > size case:
//     REQUIRE(v.size() < 20'000);
//     lf::sync_wait(pool, lf::for_each, v, 20'000, fun);
//     check(v, count++);

//     // ----- With projection ---- //

//     async inc = [](auto, int *i) -> task<> {
//       (*i)++;
//       co_return;
//     };

//     lf::sync_wait(pool, lf::for_each, v, inc, address);
//     check(v, count++);

//     lf::sync_wait(pool, lf::for_each, v, 300, inc, address);
//     check(v, count++);
//   }

//   // ----------- Now with small inputs ----------- //

//   std::vector<int> small = {0, 0, 0};

//   auto add = [](int &i) {
//     i++;
//   };

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 3), add);

//   REQUIRE(small == std::vector<int>{1, 1, 1});

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 2), add);

//   REQUIRE(small == std::vector<int>{2, 2, 1});

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 1), add);

//   REQUIRE(small == std::vector<int>{3, 2, 1});

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 0), add);

//   REQUIRE(small == std::vector<int>{3, 2, 1});

//   // ------------------ with large n ------------------ //

//   small = {0, 0, 0};

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 3), 2, add);

//   REQUIRE(small == std::vector<int>{1, 1, 1});

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 2), 2, add);

//   REQUIRE(small == std::vector<int>{2, 2, 1});

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 1), 2, add);

//   REQUIRE(small == std::vector<int>{3, 2, 1});

//   lf::sync_wait(pool, lf::for_each, std::span(small.data(), 0), 2, add);

//   REQUIRE(small == std::vector<int>{3, 2, 1});
// }

// // NOLINTEND