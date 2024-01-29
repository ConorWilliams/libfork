// MIT License

// Copyright (c) 2020 T.-W. Huang and C.J. Williams

// University of Utah, Salt Lake City, UT, USA

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// The contents of this file have been adapted from: https://github.com/taskflow/work-stealing-queue

#include <algorithm>                    // for min
#include <catch2/catch_test_macros.hpp> // for operator""_catch_sr, AssertionHandler, REQUIRE
#include <deque>                        // for deque, operator==, _Deque_iterator
#include <optional>                     // for optional
#include <set>                          // for set, operator==, _Rb_tree_const_iterator
#include <stdint.h>                     // for int64_t
#include <stdlib.h>                     // for rand
#include <thread>                       // for thread
#include <vector>                       // for vector

#include "libfork/core.hpp" // for deque, steal_t, return_nullopt

// NOLINTBEGIN No linting in tests

using namespace lf;

// Procedure: wsq_test_owner
void wsq_test_owner() {
  int64_t cap = 2;

  deque<int> deque(cap);
  std::deque<int> gold;

  REQUIRE(deque.capacity() == 2);
  REQUIRE(deque.empty());

  for (int i = 2; i <= (1 << 16); i <<= 1) {
    REQUIRE(deque.empty());

    for (int j = 0; j < i; ++j) {
      deque.push(j);
    }

    for (int j = 0; j < i; ++j) {
      auto item = deque.pop();
      REQUIRE((item && *item == i - j - 1));
    }
    REQUIRE(!deque.pop());

    REQUIRE(deque.empty());
    for (int j = 0; j < i; ++j) {
      deque.push(j);
    }

    for (int j = 0; j < i; ++j) {
      auto item = deque.steal();
      REQUIRE((item && *item == j));
    }
    REQUIRE(!deque.pop());

    REQUIRE(deque.empty());

    for (int j = 0; j < i; ++j) {
      // endeque
      if (auto dice = ::rand() % 3; dice == 0) {
        deque.push(j);
        gold.push_back(j);

      } else if (dice == 1) {
        auto item = deque.pop();
        if (gold.empty()) {
          REQUIRE(!item);
        } else {
          REQUIRE((item && *item == gold.back()));
          gold.pop_back();
        }
      } else {
        auto item = deque.steal();
        if (gold.empty()) {
          REQUIRE(!item);
        } else {
          REQUIRE((item && *item == gold.front()));
          gold.pop_front();
        }
      }

      REQUIRE((deque.size() == gold.size()));
    }

    while (!deque.empty()) {
      auto item = deque.pop();
      REQUIRE((item && *item == gold.back()));
      gold.pop_back();
    }

    REQUIRE(gold.empty());

    REQUIRE(deque.capacity() == i);
  }
}

// Procedure: wsq_test_n_thieves
void wsq_test_n_thieves(int N) {
  int64_t cap = 2;

  deque<int> deque(cap);

  REQUIRE(deque.capacity() == 2);
  REQUIRE(deque.empty());

  for (int i = 2; i <= (1 << 16); i <<= 1) {
    REQUIRE(deque.empty());

    int p = 0;

    std::vector<std::deque<int>> cdeqs(N);
    std::vector<std::thread> consumers;
    std::deque<int> pdeq;

    auto num_stolen = [&]() {
      int total = 0;
      for (const auto &cdeq : cdeqs) {
        total += static_cast<int>(cdeq.size());
      }
      return total;
    };

    for (int n = 0; n < N; n++) {
      consumers.emplace_back([&, n]() {
        while (num_stolen() + (int)pdeq.size() != i) {
          if (auto dice = ::rand() % 4; dice == 0) {
            if (auto item = deque.steal(); item) {
              cdeqs[n].push_back(*item);
            }
          }
        }
      });
    }

    std::thread producer([&]() {
      while (p < i) {
        if (auto dice = ::rand() % 4; dice == 0) {
          deque.push(p++);
        } else if (dice == 1) {
          if (auto item = deque.pop(); item) {
            pdeq.push_back(*item);
          }
        }
      }
    });

    producer.join();

    for (auto &c : consumers) {
      c.join();
    }

    REQUIRE(deque.empty());
    REQUIRE(deque.capacity() <= i);

    std::set<int> set;

    for (auto const &cdeq : cdeqs) {
      for (auto k : cdeq) {
        set.insert(k);
      }
    }

    for (auto k : pdeq) {
      set.insert(k);
    }

    for (int j = 0; j < i; ++j) {
      REQUIRE(set.find(j) != set.end());
    }

    REQUIRE((int)set.size() == i);
  }
}

// ----------------------------------------------------------------------------
// Testcase: WSQTest.Owner
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.Owner", "[wsq]") { wsq_test_owner(); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.nThief
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.nThieves", "[wsq]") {
  for (unsigned int i = 1; i <= std::min(8U, std::thread::hardware_concurrency()); ++i) {
    wsq_test_n_thieves(i);
  }
}

// NOLINTEND