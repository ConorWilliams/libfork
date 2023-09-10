// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

// !BEGIN-EXAMPLE

#include <thread>

#include "libfork/queue.hpp"

auto example() -> int {
  // Work-stealing queue of ints
  lf::queue<int> queue;

  constexpr int num_items = 10000;

  // One thread can push and pop items from one end (like a stack)
  std::thread owner([&]() {
    for (int i = 0; i < num_items; ++i) {
      queue.push(i);
    }
    while (std::optional item = queue.pop()) {
      // Do something with items...
    }
  });

  // While multiple (any) threads can steal items from the other end
  std::thread thief([&]() {
    while (!queue.empty()) {
      if (auto item = queue.steal()) {
        // Do something with item...
      }
    }
  });

  owner.join();
  thief.join();

  return 0;
}

// !END-EXAMPLE

// NOLINTBEGIN No linting in tests

TEST_CASE("Example", "[queue]") { REQUIRE(!example()); }

TEST_CASE("Single thread as stack", "[queue]") {
  lf::queue<int> queue;

  REQUIRE(queue.empty());

  for (int i = 0; i < 10; ++i) {
    queue.push(i);
    REQUIRE(queue.ssize() == i + 1);
  }

  for (int i = 9; i >= 0; --i) {
    auto item = queue.pop();
    REQUIRE(item);
    REQUIRE(*item == i);
  }

  REQUIRE(queue.empty());
}

TEST_CASE("Single producer, single consumer", "[queue]") {
  lf::queue<int> queue;

  constexpr int tot = 100;

  std::thread thief([&] {
    //
    int count = 0;

    while (count < tot) {
      if (auto [err, item] = queue.steal(); err == lf::err::none) {
        REQUIRE(item == count++);
      } else {
        REQUIRE(err == lf::err::empty);
      }
    }
  });

  for (int i = 0; i < tot; ++i) {
    queue.push(i);
  }

  thief.join();

  REQUIRE(queue.empty());
}

TEST_CASE("Single producer, multiple consumer", "[queue]") {
  lf::queue<int> queue;

  auto &worker = queue;
  auto &stealer = queue;

  constexpr auto max = 100000;
  unsigned int nthreads = std::thread::hardware_concurrency();

  std::vector<std::thread> threads;
  std::atomic<int> remaining(max);

  for (unsigned int i = 0; i < nthreads; ++i) {
    threads.emplace_back([&stealer, &remaining]() {
      auto &clone = stealer;
      while (remaining.load() > 0) {
        if (clone.steal()) {
          remaining.fetch_sub(1);
        }
      }
    });
  }

  for (auto i = 0; i < max; ++i) {
    worker.push(i);
  }

  for (auto &thr : threads) {
    thr.join();
  }

  REQUIRE(remaining == 0);
}

TEST_CASE("Single producer + pop(), multiple consumer", "[queue]") {
  lf::queue<int> queue;

  auto &worker = queue;
  auto &stealer = queue;

  constexpr auto max = 100000;
  unsigned int nthreads = std::thread::hardware_concurrency();

  std::vector<std::thread> threads;
  std::atomic<int> remaining(max);

  for (unsigned int i = 0; i < nthreads; ++i) {
    threads.emplace_back([&stealer, &remaining]() {
      auto &clone = stealer;
      while (remaining.load() > 0) {
        if (clone.steal()) {
          remaining.fetch_sub(1);
        }
      }
    });
  }

  for (auto i = 0; i < max; ++i) {
    worker.push(i);
  }

  while (remaining.load() > 0) {
    if (worker.pop()) {
      remaining.fetch_sub(1);
    }
  }

  for (auto &thr : threads) {
    thr.join();
  }

  REQUIRE(remaining == 0);
}

// NOLINTEND