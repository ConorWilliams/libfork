// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <numeric>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/utility.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

TEST_CASE("Construct and destruct", "[busy_pool]") {
  for (std::size_t n = 1; n < 100; ++n) {
    busy_pool pool{1};
  }

  for (std::size_t n = 1; n < 100; ++n) {
    busy_pool pool{2};
  }

  for (std::size_t n = 1; n < 100; ++n) {
    busy_pool pool{3};
  }

  for (std::size_t n = 1; n < 100; ++n) {
    busy_pool pool;
  }
}

template <typename T>
using task = basic_task<T, busy_pool::context>;

template <typename T>
using future = basic_future<T, busy_pool::context>;

static task<void> noop() {
  co_return;
}

template <typename T>
static task<T> fwd(T x) {
  co_return x;
}

TEST_CASE("noop", "[busy_pool]") {
  for (int i = 0; i < 100; ++i) {
    DEBUG_TRACKER("\niter");
    busy_pool pool{};
    pool.colab(noop());
  }
}

TEST_CASE("fwd", "[busy_pool]") {
  for (int i = 0; i < 100; ++i) {
    busy_pool pool{};
    REQUIRE(pool.colab(fwd(i)) == i);
  }
}

TEST_CASE("re-use", "[busy_pool]") {
  for (int i = 0; i < 100; ++i) {
    busy_pool pool{};
    for (int j = 0; j < 1; ++j) {
      REQUIRE(pool.colab(fwd(j)) == j);
    }
  }
}

// Fibonacci using recursion
static int fib(int n) {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

// Fibonacci using tasks
static task<int> fib_task(int n) {
  if (n <= 1) {
    co_return n;
  }

  auto a = co_await fib_task(n - 1).fork();
  auto b = co_await fib_task(n - 2);

  co_await join();

  co_return *a + b;
}

TEST_CASE("Fibonacci - busy_pool", "[busy_pool]") {
  busy_pool pool{};
  for (int i = 0; i < 20; ++i) {
    REQUIRE(pool.colab(fib_task(i)) == fib(i));
  }
}

template <typename T>
task<T> pool_reduce(std::span<T> range, std::size_t grain) {
  //
  if (range.size() <= grain) {
    co_return std::reduce(range.begin(), range.end());
  }

  auto head = range.size() / 2;
  auto tail = range.size() - head;

  auto a = co_await pool_reduce(range.first(head), grain).fork();
  auto b = co_await pool_reduce(range.last(tail), grain);

  co_await join();

  co_return *a + b;
}

TEST_CASE("busy reduce", "[busy_task]") {
  //
  std::vector<int> data(1 * 2 * 3 * 4 * 1024 * 1024);

  detail::xoshiro rng(std::random_device{});

  std::uniform_int_distribution<int> dist(0, 10);

  for (auto& x : data) {
    x = dist(rng);
  }

  for (std::size_t i = 2; i <= std::thread::hardware_concurrency(); ++i) {
    //
    busy_pool pool{i};

    auto result = pool.colab(pool_reduce<int>(data, data.size() / (10 * i)));

    REQUIRE(result == std::reduce(data.begin(), data.end()));
  }
}

// TEST_CASE("reduce", "[!benchmark]") {
//   //

//   std::vector<int> range(12 * 12 * 1024 * 1024);

//   xoshiro rng(std::random_device{});

//   std::uniform_int_distribution<int> dist(0, 10);

//   for (auto& x : range) {
//     x = dist(rng);
//   }

//   int correct;

//   BENCHMARK("linear reduce") {
//     correct = std::reduce(range.begin(), range.end(), 0);
//     return correct;
//   };

//   for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
//     //

//     std::size_t chunk = range.size() / (i * 3);

//     BENCHMARK("inline reduce " + std::to_string(i)) {
//       return inline_reduce(range, chunk);
//     };

//     {
//       busy_pool pool{i};

//       BENCHMARK("pool reduce " + std::to_string(i) + " thread(s)") {
//         return pool.sync_wait(pool_reduce<int>(range, chunk));
//       };
//     }

// #pragma omp parallel num_threads(i)
// #pragma omp single nowait
//     {
//       BENCHMARK("openMP tasking " + std::to_string(i) + " thread(s)") {
//         return omp_reduce(range, chunk);
//       };
//     }
//   }
// }

// NOLINTEND