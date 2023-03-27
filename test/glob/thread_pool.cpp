// // Copyright © Conor Williams <conorwilliams@outlook.com>

// // SPDX-License-Identifier: MPL-2.0

// // This Source Code Form is subject to the terms of the Mozilla Public
// // License, v. 2.0. If a copy of the MPL was not distributed with this
// // file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #include <numeric>
// #include <span>

// #include <catch2/catch_test_macros.hpp>

// #include "libfork/detail/utility.hpp"
// #include "libfork/schedule/thread_pool.hpp"

// // NOLINTBEGIN No need to check the tests for style.

// using namespace lf;

// // TEST_CASE("Construct and destruct", "[thread_pool]") {
// //   for (std::size_t n = 1; n < 100; ++n) {
// //     thread_pool pool{1};
// //   }

// //   for (std::size_t n = 1; n < 100; ++n) {
// //     thread_pool pool{2};
// //   }

// //   for (std::size_t n = 1; n < 100; ++n) {
// //     thread_pool pool{3};
// //   }

// //   for (std::size_t n = 1; n < 100; ++n) {
// //     thread_pool pool;
// //   }
// // }

// template <typename T>
// using task = basic_task<T, thread_pool::context>;

// template <typename T>
// using future = basic_future<T, thread_pool::context>;

// static task<void> noop() {
//   co_return;
// }

// template <typename T>
// static task<T> fwd(T x) {
//   co_return x;
// }

// TEST_CASE("noop", "[thread_pool]") {
//   for (int i = 0; i < 1; ++i) {
//     DEBUG_TRACKER("iter\n");
//     thread_pool pool{2};
//     pool.schedule(noop());
//     DEBUG_TRACKER("done");
//   }
//   exit(0);
// }

// TEST_CASE("fwd", "[thread_pool]") {
//   for (int i = 0; i < 100; ++i) {
//     thread_pool pool{};
//     REQUIRE(pool.schedule(fwd(i)) == i);
//   }
// }

// TEST_CASE("re-use", "[thread_pool]") {
//   for (int i = 0; i < 100; ++i) {
//     thread_pool pool{};
//     for (int j = 0; j < 1; ++j) {
//       REQUIRE(pool.schedule(fwd(j)) == j);
//     }
//   }
// }

// // Fibonacci using recursion
// static int fib(int n) {
//   if (n <= 1) {
//     return n;
//   }
//   return fib(n - 1) + fib(n - 2);
// }

// // Fibonacci using tasks
// static task<int> fib_task(int n) {
//   if (n <= 1) {
//     co_return n;
//   }

//   auto a = co_await fib_task(n - 1).fork();
//   auto b = co_await fib_task(n - 2);

//   co_await join();

//   co_return *a + b;
// }

// TEST_CASE("Fibonacci - thread_pool", "[thread_pool]") {
//   thread_pool pool{2};
//   for (int i = 2; i < 3; ++i) {
//     REQUIRE(pool.schedule(fib_task(i)) == fib(i));
//   }
// }

// template <typename T>
// task<T> pool_reduce(std::span<T> range, std::size_t grain) {
//   //
//   if (range.size() <= grain) {
//     co_return std::reduce(range.begin(), range.end());
//   }

//   auto head = range.size() / 2;
//   auto tail = range.size() - head;

//   auto a = co_await pool_reduce(range.first(head), grain).fork();
//   auto b = co_await pool_reduce(range.last(tail), grain);

//   co_await join();

//   co_return *a + b;
// }

// TEST_CASE("busy reduce", "[thread_pool]") {
//   //
//   std::vector<int> data(1 * 2 * 3 * 4 * 1024 * 1024);

//   detail::xoshiro rng(std::random_device{});

//   std::uniform_int_distribution<int> dist(0, 10);

//   for (auto& x : data) {
//     x = dist(rng);
//   }

//   for (std::size_t i = 2; i <= std::thread::hardware_concurrency(); ++i) {
//     //
//     thread_pool pool{i};

//     auto result = pool.schedule(pool_reduce<int>(data, data.size() / (10 * i)));

//     REQUIRE(result == std::reduce(data.begin(), data.end()));
//   }
// }

// TEST_CASE("brute force", "[thread_pool]") {
//   //
//   lf::thread_pool pool{2};

//   auto f_5 = fib(5);

//   for (std::size_t i = 1; i <= 10'000; ++i) {
//     REQUIRE(pool.schedule(fib_task(5)) == f_5);
//   }
// }

// // NOLINTEND