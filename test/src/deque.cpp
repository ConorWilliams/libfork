#include <catch2/catch_test_macros.hpp>

import std;

import libfork.core;

using namespace lf;

TEST_CASE("Deque: Concepts", "[deque]") {
  STATIC_REQUIRE(dequeable<int>);
  STATIC_REQUIRE(dequeable<double>);
  STATIC_REQUIRE_FALSE(dequeable<std::string>); // Not trivially copyable
}

TEST_CASE("Deque: Single thread as stack", "[deque]") {
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

TEST_CASE("Deque: Custom pop when_empty", "[deque]") {
  lf::deque<int> deque;

  auto result = deque.pop([]() { return -1; });
  REQUIRE(result == -1);

  deque.push(42);
  result = deque.pop([]() { return -1; });
  REQUIRE(result == 42);

  result = deque.pop([]() { return -1; });
  REQUIRE(result == -1);
}

TEST_CASE("Deque: Single producer, single consumer", "[deque]") {
  lf::deque<int> deque;

  constexpr int tot = 100;

  std::thread thief([&] {
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

TEST_CASE("Deque: Single producer, multiple consumer", "[deque]") {
  lf::deque<int> deque;

  constexpr auto max = 10000;
  unsigned int nthreads = std::thread::hardware_concurrency();

  std::vector<std::thread> threads;
  std::atomic<int> remaining(max);

  for (unsigned int i = 0; i < nthreads; ++i) {
    threads.emplace_back([&deque, &remaining]() {
      while (remaining.load() > 0) {
        if (deque.steal()) {
          remaining.fetch_sub(1);
        }
      }
    });
  }

  for (auto i = 0; i < max; ++i) {
    deque.push(i);
  }

  for (auto &thr : threads) {
    thr.join();
  }

  REQUIRE(remaining == 0);
  REQUIRE(deque.empty());
}

TEST_CASE("Deque: Single producer + pop(), multiple consumer", "[deque]") {
  lf::deque<int> deque;

  constexpr auto max = 10000;
  unsigned int nthreads = std::thread::hardware_concurrency();

  std::vector<std::thread> threads;
  std::atomic<int> remaining(max);

  for (unsigned int i = 0; i < nthreads; ++i) {
    threads.emplace_back([&deque, &remaining]() {
      while (remaining.load() > 0) {
        if (deque.steal()) {
          remaining.fetch_sub(1);
        }
      }
    });
  }

  for (auto i = 0; i < max; ++i) {
    deque.push(i);
  }

  while (remaining.load() > 0) {
    if (deque.pop()) {
      remaining.fetch_sub(1);
    }
  }

  for (auto &thr : threads) {
    thr.join();
  }

  REQUIRE(remaining == 0);
  REQUIRE(deque.empty());
}

namespace {

void wsq_test_owner() {
  int64_t cap = 2;

  lf::deque<std::size_t> deque(cap);
  std::deque<std::size_t> gold;

  REQUIRE(deque.capacity() == 2);
  REQUIRE(deque.empty());

  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, 2);

  for (std::size_t i = 2; i <= (1 << 14); i <<= 1) {
    REQUIRE(deque.empty());

    for (std::size_t j = 0; j < i; ++j) {
      deque.push(j);
    }

    for (std::size_t j = 0; j < i; ++j) {
      auto item = deque.pop();
      REQUIRE((item && *item == i - j - 1));
    }
    REQUIRE(!deque.pop());

    REQUIRE(deque.empty());
    for (std::size_t j = 0; j < i; ++j) {
      deque.push(j);
    }

    for (std::size_t j = 0; j < i; ++j) {
      auto item = deque.steal();
      REQUIRE((item && *item == j));
    }
    REQUIRE(!deque.pop());

    REQUIRE(deque.empty());

    for (std::size_t j = 0; j < i; ++j) {
      int dice = dis(gen);
      if (dice == 0) {
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
      REQUIRE(deque.size() == gold.size());
    }

    while (!deque.empty()) {
      auto item = deque.pop();
      REQUIRE((item && *item == gold.back()));
      gold.pop_back();
    }

    REQUIRE(gold.empty());
    REQUIRE(static_cast<std::size_t>(deque.capacity()) == i);
  }
}

void wsq_test_n_thieves(std::size_t N) {
  int64_t cap = 2;
  lf::deque<std::size_t> deque(cap);

  REQUIRE(deque.capacity() == 2);
  REQUIRE(deque.empty());

  for (std::size_t i = 2; i <= (1 << 14); i <<= 1) {
    REQUIRE(deque.empty());

    std::atomic<std::size_t> p = 0;
    std::vector<std::deque<std::size_t>> cdeqs(N);
    std::vector<std::thread> consumers;
    std::deque<std::size_t> pdeq;
    std::mutex pdeq_mutex;

    std::atomic<std::size_t> total_recovered = 0;

    for (std::size_t n = 0; n < N; n++) {
      consumers.emplace_back([&, n, i]() {
        std::mt19937 gen(1337 + n);
        std::uniform_int_distribution<> dis(0, 3);
        while (total_recovered.load() < i) {
          if (dis(gen) == 0) {
            if (auto item = deque.steal(); item) {
              cdeqs[n].push_back(*item);
              total_recovered.fetch_add(1);
            }
          }
          std::this_thread::yield();
        }
      });
    }

    std::thread producer([&]() {
      std::mt19937 gen(42);
      std::uniform_int_distribution<> dis(0, 3);
      std::size_t pushed = 0;
      while (pushed < i || total_recovered.load() < i) {
        int dice = dis(gen);
        if (dice == 0 && pushed < i) {
          deque.push(pushed++);
        } else if (dice == 1) {
          if (auto item = deque.pop(); item) {
            std::lock_guard lock(pdeq_mutex);
            pdeq.push_back(*item);
            total_recovered.fetch_add(1);
          }
        }
        std::this_thread::yield();
      }
    });

    producer.join();
    for (auto &c : consumers) {
      c.join();
    }

    REQUIRE(deque.empty());
    REQUIRE(static_cast<std::size_t>(deque.capacity()) <= i);

    std::set<std::size_t> set;
    for (auto const &cdeq : cdeqs) {
      for (auto k : cdeq) {
        set.insert(k);
      }
    }
    for (auto k : pdeq) {
      set.insert(k);
    }

    for (std::size_t j = 0; j < i; ++j) {
      REQUIRE(set.contains(j));
    }
    REQUIRE(set.size() == i);
  }
}

} // namespace

TEST_CASE("Deque: WSQ Owner Intensive", "[deque][wsq]") { wsq_test_owner(); }

TEST_CASE("Deque: WSQ nThieves Intensive", "[deque][wsq]") {
  for (unsigned int i = 1; i <= std::min(4U, std::thread::hardware_concurrency()); ++i) {
    wsq_test_n_thieves(i);
  }
}
