// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <numeric>
#include <span>

#define NLOG
#define NDEBUG

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/forkpool.hpp"
#include "libfork/random.hpp"
#include "libfork/utility.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

TEST_CASE("Construct and destruct", "[forkpool]") {
  for (std::size_t n = 1; n < 100; ++n) {
    forkpool pool{1};
  }

  for (std::size_t n = 1; n < 100; ++n) {
    forkpool pool{2};
  }

  for (std::size_t n = 1; n < 100; ++n) {
    forkpool pool;
  }
}

task<int, forkpool::context> pool_fib(int x) {
  //
  if (x < 2) {
    co_return x;
  }

  future<int> a, b;

  co_await fork(a, pool_fib, x - 1);
  co_await just(b, pool_fib, x - 2);

  co_await join();

  co_return a + b;
}

int fib_recursive(int x) {
  //
  if (x < 2) {
    return x;
  }

  return fib_recursive(x - 1) + fib_recursive(x - 2);
}

TEST_CASE("pool_fibonacci", "[forkpool]") {
  //
  forkpool pool;

  for (std::size_t n = 1; n < 25; ++n) {
    REQUIRE(pool.sync_wait(pool_fib(n)) == fib_recursive(n));
  }
}

template <typename T>
task<T, forkpool::context> pool_reduce(std::span<T> range, std::size_t grain) {
  //
  if (range.size() <= grain) {
    co_return std::reduce(range.begin(), range.end());
  }

  future<T> a, b;

  auto head = range.size() / 2;
  auto tail = range.size() - head;

  co_await fork(a, pool_reduce(range.first(head), grain));
  co_await just(b, pool_reduce(range.last(tail), grain));

  co_await join();

  co_return a + b;
}

int omp_reduce(std::span<int> range, std::size_t grain) {
  //
  if (range.size() <= grain) {
    return std::reduce(range.begin(), range.end());
  }

  int a, b;

  auto head = range.size() / 2;
  auto tail = range.size() - head;

#pragma omp task untied default(none) shared(a, range) firstprivate(grain, head)
  a = omp_reduce(range.first(head), grain);

  b = omp_reduce(range.last(tail), grain);

#pragma omp taskwait

  return a + b;
}

int inline_reduce(std::span<int> range, std::size_t grain) {
  //
  if (range.size() <= grain) {
    return std::reduce(range.begin(), range.end());
  }

  auto head = range.size() / 2;
  auto tail = range.size() - head;

  int a = inline_reduce(range.first(head), grain);
  int b = inline_reduce(range.last(tail), grain);

  return a + b;
}

TEST_CASE("reduce", "[!benchmark]") {
  //

  std::vector<int> range(12 * 12 * 1024 * 1024);

  xoshiro rng(std::random_device{});

  std::uniform_int_distribution<int> dist(0, 10);

  for (auto& x : range) {
    x = dist(rng);
  }

  int correct;

  BENCHMARK("linear reduce") {
    correct = std::reduce(range.begin(), range.end(), 0);
    return correct;
  };

  for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //

    std::size_t chunk = range.size() / (i * 3);

    BENCHMARK("inline reduce " + std::to_string(i)) {
      return inline_reduce(range, chunk);
    };

    {
      forkpool pool{i};

      BENCHMARK("pool reduce " + std::to_string(i) + " thread(s)") {
        return pool.sync_wait(pool_reduce<int>(range, chunk));
      };
    }

#pragma omp parallel num_threads(i)
#pragma omp single nowait
    {
      BENCHMARK("openMP tasking " + std::to_string(i) + " thread(s)") {
        return omp_reduce(range, chunk);
      };
    }
  }
}

// NOLINTEND