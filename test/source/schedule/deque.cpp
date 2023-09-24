// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

// !BEGIN-EXAMPLE

#include <thread>

#include "libfork/schedule/deque.hpp"

auto example() -> int {
  // Work-stealing deque of ints
  lf::deque<int> deque;

  constexpr int num_items = 10000;

  // One thread can push and pop items from one end (like a stack)
  std::thread owner([&]() {
    for (int i = 0; i < num_items; ++i) {
      deque.push(i);
    }
    while (std::optional item = deque.pop()) {
      // Do something with items...
    }
  });

  // While multiple (any) threads can steal items from the other end
  std::thread thief([&]() {
    while (!deque.empty()) {
      if (auto item = deque.steal()) {
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

TEST_CASE("Example", "[deque]") { REQUIRE(!example()); }

TEST_CASE("Single thread as stack", "[deque]") {
  lf::deque<int> deque;

  REQUIRE(deque.empty());

  for (int i = 0; i < 10; ++i) {
    deque.push(i);
    REQUIRE(deque.ssize() == i + 1);
  }

  for (int i = 9; i >= 0; --i) {
    auto item = deque.pop();
    REQUIRE(item);
    REQUIRE(*item == i);
  }

  REQUIRE(deque.empty());
}

TEST_CASE("Single producer, single consumer", "[deque]") {
  lf::deque<int> deque;

  constexpr int tot = 100;

  std::thread thief([&] {
    //
    int count = 0;

    while (count < tot) {
      if (auto [err, item] = deque.steal(); err == lf::err::none) {
        REQUIRE(item == count++);
      } else {
        REQUIRE(err == lf::err::empty);
      }
    }
  });

  for (int i = 0; i < tot; ++i) {
    deque.push(i);
  }

  thief.join();

  REQUIRE(deque.empty());
}

TEST_CASE("Single producer, multiple consumer", "[deque]") {
  lf::deque<int> deque;

  auto &worker = deque;
  auto &stealer = deque;

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

TEST_CASE("Single producer + pop(), multiple consumer", "[deque]") {

  lf::deque<int> deque;

  auto &worker = deque;
  auto &stealer = deque;

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