// The contents of this file are from: https://github.com/taskflow/work-stealing-queue

#include <atomic>
#include <deque>
#include <queue>
#include <random>
#include <set>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "libfork/queue.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

// Procedure: wsq_test_owner
void wsq_test_owner() {
  int64_t cap = 2;

  queue<int> queue(cap);
  std::deque<int> gold;

  REQUIRE(queue.capacity() == 2);
  REQUIRE(queue.empty());

  for (int i = 2; i <= (1 << 16); i <<= 1) {
    REQUIRE(queue.empty());

    for (int j = 0; j < i; ++j) {
      queue.push(j);
    }

    for (int j = 0; j < i; ++j) {
      auto item = queue.pop();
      REQUIRE((item && *item == i - j - 1));
    }
    REQUIRE(!queue.pop());

    REQUIRE(queue.empty());
    for (int j = 0; j < i; ++j) {
      queue.push(j);
    }

    for (int j = 0; j < i; ++j) {
      auto item = queue.steal();
      REQUIRE((item && *item == j));
    }
    REQUIRE(!queue.pop());

    REQUIRE(queue.empty());

    for (int j = 0; j < i; ++j) {
      // enqueue
      if (auto dice = ::rand() % 3; dice == 0) {
        queue.push(j);
        gold.push_back(j);

      } else if (dice == 1) {
        auto item = queue.pop();
        if (gold.empty()) {
          REQUIRE(!item);
        } else {
          REQUIRE((item && *item == gold.back()));
          gold.pop_back();
        }
      } else {
        auto item = queue.steal();
        if (gold.empty()) {
          REQUIRE(!item);
        } else {
          REQUIRE((item && *item == gold.front()));
          gold.pop_front();
        }
      }

      REQUIRE((queue.size() == gold.size()));
    }

    while (!queue.empty()) {
      auto item = queue.pop();
      REQUIRE((item && *item == gold.back()));
      gold.pop_back();
    }

    REQUIRE(gold.empty());

    REQUIRE(queue.capacity() == i);
  }
}

// Procedure: wsq_test_n_thieves
void wsq_test_n_thieves(int N) {
  int64_t cap = 2;

  queue<int> queue(cap);

  REQUIRE(queue.capacity() == 2);
  REQUIRE(queue.empty());

  for (int i = 2; i <= (1 << 16); i <<= 1) {
    REQUIRE(queue.empty());

    int p = 0;

    std::vector<std::deque<int>> cdeqs(N);
    std::vector<std::thread> consumers;
    std::deque<int> pdeq;

    auto num_stolen = [&]() {
      int total = 0;
      for (const auto& cdeq : cdeqs) {
        total += static_cast<int>(cdeq.size());
      }
      return total;
    };

    for (int n = 0; n < N; n++) {
      consumers.emplace_back([&, n]() {
        while (num_stolen() + (int)pdeq.size() != i) {
          if (auto dice = ::rand() % 4; dice == 0) {
            if (auto item = queue.steal(); item) {
              cdeqs[n].push_back(*item);
            }
          }
        }
      });
    }

    std::thread producer([&]() {
      while (p < i) {
        if (auto dice = ::rand() % 4; dice == 0) {
          queue.push(p++);
        } else if (dice == 1) {
          if (auto item = queue.pop(); item) {
            pdeq.push_back(*item);
          }
        }
      }
    });

    producer.join();

    for (auto& c : consumers) {
      c.join();
    }

    REQUIRE(queue.empty());
    REQUIRE(queue.capacity() <= i);

    std::set<int> set;

    for (auto const& cdeq : cdeqs) {
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
TEST_CASE("WSQ.Owner", "[wsq]") {
  wsq_test_owner();
}

// ----------------------------------------------------------------------------
// Testcase: WSQTest.nThief
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.nThieves", "[wsq]") {
  for (unsigned int i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    wsq_test_n_thieves(i);
  }
}

// NOLINTEND