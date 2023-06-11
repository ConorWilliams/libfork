// // Copyright © Conor Williams <conorwilliams@outlook.com>

// // SPDX-License-Identifier: MPL-2.0

// // This Source Code Form is subject to the terms of the Mozilla Public
// // License, v. 2.0. If a copy of the MPL was not distributed with this
// // file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #include <iostream>
// #include <memory>
// #include <new>
// #include <semaphore>
// #include <stack>
// #include <type_traits>
// #include <utility>

// #include <catch2/benchmark/catch_benchmark.hpp>
// #include <catch2/catch_test_macros.hpp>

// // #define LIBFORK_LOGGING

// #include "libfork/busy_pool.hpp"
// #include "libfork/libfork.hpp"

// // NOLINTBEGIN

// using namespace lf;

// template <typename T>
// using ptask = task<T, typename lf::detail::busy_pool::worker_context>;

// inline constexpr auto fib = fn([](auto fib, int n) static -> ptask<int> {
//   //
//   if (n < 2) {
//     co_return n;
//   }

//   int a, b;

//   co_await lf::fork(a, fib)(n - 1);
//   co_await lf::call(b, fib)(n - 2);

//   co_await lf::join;

//   co_return a + b;
// });

// __attribute__((noinline)) inline int fib_2(int n) {
//   if (n < 2) {
//     return n;
//   }
//   return fib_2(n - 1) + fib_2(n - 2);
// };

// inline constexpr auto fib2 = fn([](auto, int n) static -> ptask<int> {
//   co_return fib_2(n);
// });

// inline constexpr auto noop = fn([](auto, int x) static -> ptask<int> {
//   co_return x;
// });

// TEST_CASE("fib_2") {

//   volatile int x = 10;

//   detail::busy_pool pool{2};

//   BENCHMARK("no coro") {
//     return fib_2(x);
//   };

//   BENCHMARK("noop") {
//     return pool.schedule(noop, x);
//   };

//   BENCHMARK("one coro") {
//     return pool.schedule(fib2, x);
//   };

//   BENCHMARK("pool") {

//     return pool.schedule(fib, x);
//   };
// }

// TEST_CASE("busy_pool", "[libfork]") {

//   for (int i = 0; i < 20; ++i) {

//     detail::busy_pool pool{};

//     REQUIRE(pool.schedule(fib, i) == fib_2(i));
//   }
// }

// // inline constexpr auto reduce = fn([](auto fib, int n) static -> Task<int> {
// //   //
// //   if (n < 2) {
// //     co_return n;
// //   }

// //   int a, b;

// //   co_await lf::fork[a, fib](n - 1);
// //   co_await lf::call[b, fib](n - 2);

// //   co_await lf::call[a, reduce<Context>](vec);

// //   // co_await lf::call[reduce]

// //   co_await lf::fork[a, mem_fn](a, a);

// //   co_await lf::fork[a, my_class::mem](inst, n - 1);

// //   co_await self.join;

// //   co_return a + b;
// // });

// // NOLINTEND