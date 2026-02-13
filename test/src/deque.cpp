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

  auto result = deque.pop([]() {
    return -1;
  });
  REQUIRE(result == -1);

  deque.push(42);
  result = deque.pop([]() {
    return -1;
  });
  REQUIRE(result == 42);

  result = deque.pop([]() {
    return -1;
  });
  REQUIRE(result == -1);
}

namespace {

void test_deque(std::size_t n_pushes, std::size_t n_consumers, bool do_pop) {

  // The tested queue
  lf::deque<std::uint64_t> deque{};

  // To store removed elements
  std::vector<std::uint64_t> pops{};
  std::vector<std::vector<std::uint64_t>> steals(n_consumers);

  // Sync
  std::atomic_flag done{};
  std::latch start{static_cast<std::ptrdiff_t>(n_consumers + 1)};

  //
  std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<std::size_t> dist{0, 4};

  std::vector<std::thread> consumers;

  // Consumers steal items and write to their respective vectors
  for (std::size_t i = 0; i < n_consumers; ++i) {
    consumers.emplace_back([&, i] {
      // Wait for initialization to complete
      start.arrive_and_wait();

      for (;;) {
        auto [err, item] = deque.steal();

        switch (err) {
          case lf::err::none:
            steals[i].push_back(item);
            [[fallthrough]];
          case lf::err::lost:
            break;
          case lf::err::empty:
            if (done.test()) {
              return;
            }
            std::this_thread::yield();
        }
      }
    });
  }

  // Setup, producer write n/2 items
  for (std::size_t i = 0; i < n_pushes / 2; ++i) {
    deque.push(i);
  }

  // Start the consumers
  start.arrive_and_wait();

  // Push remaining items
  if (do_pop) {
    for (std::size_t i = n_pushes / 2; i < n_pushes;) {
      if (dist(rng) == 0) {
        if (auto item = deque.pop()) {
          pops.push_back(*item);
        }
      } else {
        deque.push(i);
        ++i;
      }
    }
  } else {
    for (std::size_t i = n_pushes / 2; i < n_pushes; ++i) {
      deque.push(i);
    }
  }

  // Drain the queue
  if (do_pop) {
    for (;;) {
      if (auto item = deque.pop()) {
        pops.push_back(*item);
      } else {
        if (done.test_and_set()) {
          break;
        }
      }
    }
  } else {
    while (!deque.empty()) {
      std::this_thread::yield();
    }
    done.test_and_set();
  }

  // Stop the consumers
  for (auto &c : consumers) {
    c.join();
  }

  // Verify ascending for each thief
  for (auto &&deq : steals) {
    REQUIRE(std::ranges::is_sorted(deq));
  }

  // Accumulate all items
  std::vector<std::uint64_t> all = pops;

  for (auto &&deq : steals) {
    all.insert(all.end(), deq.begin(), deq.end());
  }

  REQUIRE(all.size() == n_pushes);

  std::ranges::sort(all);

  for (std::size_t i = 0; i < n_pushes; ++i) {
    REQUIRE(all[i] == i);
  }
}

constexpr std::size_t max_elements = 1uz << 20;

} // namespace

TEST_CASE("Deque: Single threaded", "[deque]") {
  for (std::size_t i = 1; i <= max_elements; i <<= 1) {
    DYNAMIC_SECTION("Elements: " << i) { test_deque(i, 0, true); }
  }
}

TEST_CASE("Deque: SPSC no-pop", "[deque]") {
  for (std::size_t i = 1; i <= max_elements; i <<= 1) {
    DYNAMIC_SECTION("Elements: " << i) { test_deque(i, 1, false); }
  }
}

TEST_CASE("Deque: SPSC with-pop", "[deque]") {
  for (std::size_t i = 1; i <= max_elements; i <<= 1) {
    DYNAMIC_SECTION("Elements: " << i) { test_deque(i, 1, true); }
  }
}

TEST_CASE("Deque: MPSC no-pop", "[deque]") {
  unsigned int max_threads = std::min(4u, std::thread::hardware_concurrency());
  for (std::size_t i = 1; i <= (max_elements >> 2); i <<= 1) {
    for (std::size_t j = 2; j <= max_threads; ++j) {
      DYNAMIC_SECTION("Elements: " << i << " Consumers: " << j) { test_deque(i, j, false); }
    }
  }
}

TEST_CASE("Deque: MPSC with-pop", "[deque]") {
  unsigned int max_threads = std::min(4u, std::thread::hardware_concurrency());
  for (std::size_t i = 1; i <= (max_elements >> 2); i <<= 1) {
    for (std::size_t j = 2; j <= max_threads; ++j) {
      DYNAMIC_SECTION("Elements: " << i << " Consumers: " << j) { test_deque(i, j, true); }
    }
  }
}

TEST_CASE("Deque: Capacity Growth", "[deque]") {
  lf::deque<int> d(2);
  REQUIRE(d.capacity() == 2);
  d.push(1);
  d.push(2);
  REQUIRE(d.capacity() == 2);
  d.push(3); // Should trigger resize
  REQUIRE(d.capacity() > 2);
  REQUIRE(d.ssize() == 3);
  REQUIRE(*d.pop() == 3);
  REQUIRE(*d.pop() == 2);
  REQUIRE(*d.pop() == 1);
}
