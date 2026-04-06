#include <catch2/catch_test_macros.hpp>

import std;
import libfork;

using lf::k_new_align;
using lf::worker_stack;

namespace {

auto not_constexpr() {}

} // namespace

#define expect(expr)                                                                                         \
  if consteval {                                                                                             \
    if (!(expr)) {                                                                                           \
      not_constexpr();                                                                                       \
    }                                                                                                        \
  } else {                                                                                                   \
    REQUIRE(expr);                                                                                           \
  }

#define TEST_CONSTEXPR(...)                                                                                  \
  constexpr auto impl = __VA_ARGS__;                                                                         \
  STATIC_REQUIRE(impl());                                                                                    \
  REQUIRE(impl())

namespace {

constexpr void check_alignment(void *ptr) {

  expect(ptr != nullptr);

  if !consteval {
    REQUIRE(lf::is_sufficiently_aligned<k_new_align>(ptr));
  }
}

} // namespace

TEST_CASE("Concept", "[geometric_stack]") {
  STATIC_REQUIRE(worker_stack<lf::geometric_stack<>>); //
}

TEST_CASE("Basic push and pop", "[geometric_stack]") {
  TEST_CONSTEXPR([]() -> bool {
    lf::geometric_stack<> stack;
    expect(stack.empty());

    void *p1 = stack.push(10);
    check_alignment(p1);
    expect(!stack.empty());

    void *p2 = stack.push(20);
    check_alignment(p2);
    expect(p2 != p1);
    expect(!stack.empty());

    // Pop in FILO order
    stack.pop(p2, 20);
    stack.pop(p1, 10);
    expect(stack.empty());

    return true;
  });
}

TEST_CASE("Checkpoint and Acquire/Release", "[geometric_stack]") {
  TEST_CONSTEXPR([]() -> bool {
    lf::geometric_stack<> stack1;
    void *p1 = stack1.push(100);
    auto cp1 = stack1.checkpoint();

    lf::geometric_stack<> stack2;
    auto cp2 = stack2.checkpoint();
    expect(cp1 != cp2);

    auto key1 = stack1.prepare_release();
    stack2.acquire(cp1);
    stack1.release(key1);
    expect(stack2.checkpoint() == cp1);
    stack2.pop(p1, 100);

    return true;
  });
}

TEST_CASE("Stress test", "[geometric_stack]") {
  for (int k = 0; k < 10; ++k) {

    lf::geometric_stack<> stack;
    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> size_dist{1, 200};
    std::uniform_int_distribution<std::size_t> depth_dist{5, 5000};

    struct entry {
      void *ptr;
      std::size_t size;
    };

    // Perform several rounds of deep push/pop sequences
    for (int i = 0; i < 2; ++i) {
      std::vector<entry> entries;
      const std::size_t depth = depth_dist(rng);

      // Push phase
      for (std::size_t j = 0; j < depth; ++j) {
        std::size_t s = size_dist(rng);
        void *p = stack.push(s);
        check_alignment(p);
        entries.push_back({.ptr = p, .size = s});
      }

      // Pop phase (FILO)
      for (std::size_t j = depth; j > 0; --j) {
        auto const &e = entries[j - 1];
        stack.pop(e.ptr, e.size);
      }

      REQUIRE(stack.empty());
    }
  }
}

TEST_CASE("Randomized push/pop stress test", "[geometric_stack]") {
  lf::geometric_stack<> stack;
  std::mt19937_64 rng{std::random_device{}()};
  std::bernoulli_distribution push_dist{0.51};
  std::uniform_int_distribution<std::size_t> size_dist{1, 512};

  struct entry {
    void *ptr;
    std::size_t size;
  };
  std::vector<entry> entries;
  std::size_t total_pushed = 0;
  const std::size_t target_pushed = 10'000'000;

  while (total_pushed < target_pushed) {

    if (entries.empty()) {
      REQUIRE(stack.empty());
    }

    if (entries.empty() || push_dist(rng)) {
      std::size_t s = size_dist(rng);
      void *p = stack.push(s);
      check_alignment(p);
      entries.push_back({.ptr = p, .size = s});
      total_pushed++;
    } else {
      auto e = entries.back();
      stack.pop(e.ptr, e.size);
      entries.pop_back();
    }
  }

  // Clean up remaining entries
  while (!entries.empty()) {
    auto e = entries.back();
    stack.pop(e.ptr, e.size);
    entries.pop_back();
  }

  REQUIRE(stack.empty());
}

TEST_CASE("Spikey randomized push/pop stress test", "[geometric_stack]") {
  lf::geometric_stack<> stack;
  std::mt19937_64 rng{std::random_device{}()};

  // Higher probability of push after push, higher probability of pop after pop
  std::bernoulli_distribution push_after_push{0.95};
  std::bernoulli_distribution push_after_pop{0.1};
  std::uniform_int_distribution<std::size_t> size_dist{1, 512};

  struct entry {
    void *ptr;
    std::size_t size;
  };
  std::vector<entry> entries;
  std::size_t total_pushed = 0;
  const std::size_t target_pushed = 10'000'000;
  bool last_was_push = true;

  while (total_pushed < target_pushed) {

    bool do_push = true;

    if (entries.empty()) {
      REQUIRE(stack.empty());
      do_push = true;
    } else if (last_was_push) {
      do_push = push_after_push(rng);
    } else {
      do_push = push_after_pop(rng);
    }

    if (do_push) {
      std::size_t s = size_dist(rng);
      void *p = stack.push(s);
      check_alignment(p);
      entries.push_back({.ptr = p, .size = s});
      total_pushed++;
      last_was_push = true;
    } else {
      auto e = entries.back();
      stack.pop(e.ptr, e.size);
      entries.pop_back();
      last_was_push = false;
    }
  }

  // Clean up remaining entries
  while (!entries.empty()) {
    auto e = entries.back();
    stack.pop(e.ptr, e.size);
    entries.pop_back();
  }
  REQUIRE(stack.empty());
}
