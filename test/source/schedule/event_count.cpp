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

// The contents of this file have been adapted from https://github.com/facebook/folly

#include <algorithm>
#include <atomic>
#include <random>
#include <semaphore>
#include <thread>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libfork/schedule/ext/event_count.hpp"
#include "libfork/schedule/ext/random.hpp"

// NOLINTBEGIN No need to check the tests for style.

namespace {

template <class T, class Random>
void randomPartition(Random &random, T key, int n, std::vector<std::pair<T, int>> &out) {
  while (n != 0) {
    int m = std::min(n, 1000);
    std::uniform_int_distribution<uint32_t> u(1, m);
    int cut = u(random);
    out.emplace_back(key, cut);
    n -= cut;
  }
}

class semaphore {
 public:
  explicit semaphore(int v = 0) : value_(v) {}

  void down() {
    ec_.await([this] {
      return tryDown();
    });
  }

  void up() {
    ++value_;
    ec_.notify_all();
  }

  int value() const { return value_; }

 private:
  bool tryDown() {
    for (int v = value_; v != 0;) {
      if (value_.compare_exchange_weak(v, v - 1)) {
        return true;
      }
    }
    return false;
  }

  std::atomic<int> value_;
  lf::event_count ec_;
};

} // namespace

TEST_CASE("event count, deadlocking (folly)", "[event_count]") {
  // We're basically testing for no deadlock.
  static const size_t count = 300000;

  enum class Op {
    UP,
    DOWN,
  };

  std::vector<std::pair<Op, int>> ops;
  lf::xoshiro rnd(lf::seed, std::random_device{});

  randomPartition(rnd, Op::UP, count, ops);
  // size_t uppers = ops.size();
  randomPartition(rnd, Op::DOWN, count, ops);
  // size_t downers = ops.size() - uppers;

  std::shuffle(ops.begin(), ops.end(), rnd);

  std::vector<std::thread> threads;
  threads.reserve(ops.size());

  semaphore sem;

  for (auto &op : ops) {
    int n = op.second;
    if (op.first == Op::UP) {
      auto fn = [&sem, n]() mutable {
        while (n--) {
          sem.up();
        }
      };
      threads.push_back(std::thread(fn));
    } else {
      auto fn = [&sem, n]() mutable {
        while (n--) {
          sem.down();
        }
      };
      threads.push_back(std::thread(fn));
    }
  }

  for (auto &thread : threads) {
    thread.join();
  }

  REQUIRE(0 == sem.value());
}

// NOLINTEND