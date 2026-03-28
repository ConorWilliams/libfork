#include <catch2/catch_test_macros.hpp>

import std;
import libfork.core;

using namespace lf;

TEST_CASE("Stack: Concepts", "[stack]") { STATIC_REQUIRE(stack_allocator<geometric_stack>); }

TEST_CASE("Stack: Basic push and pop", "[stack]") {
  geometric_stack stack;

  void *p1 = stack.push(10);
  REQUIRE(p1 != nullptr);
  REQUIRE((reinterpret_cast<std::uintptr_t>(p1) % k_new_align) == 0);

  void *p2 = stack.push(20);
  REQUIRE(p2 != nullptr);
  REQUIRE((reinterpret_cast<std::uintptr_t>(p2) % k_new_align) == 0);
  REQUIRE(p2 != p1);

  // Pop in FILO order
  stack.pop(p2, 20);
  stack.pop(p1, 10);
}

TEST_CASE("Stack: Checkpoint and Acquire/Release", "[stack]") {
  geometric_stack stack1;
  void *p1 = stack1.push(100);
  auto cp1 = stack1.checkpoint();

  geometric_stack stack2;
  auto cp2 = stack2.checkpoint();
  REQUIRE(cp1 != cp2);

  SECTION("Acquire other stack") {
    auto key1 = stack1.prepare_release();
    stack1.release(key1);
    stack2.acquire(cp1);
    REQUIRE(stack2.checkpoint() == cp1);
    stack2.pop(p1, 100);
  }
}

TEST_CASE("Stack: Stress test", "[stack]") {

  for (int k = 0; k < 10; ++k) {

    geometric_stack stack;
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
        REQUIRE(p != nullptr);
        REQUIRE((reinterpret_cast<std::uintptr_t>(p) % k_new_align) == 0);
        entries.push_back({p, s});
      }

      // Pop phase (FILO)
      for (std::size_t j = depth; j > 0; --j) {
        auto const &e = entries[j - 1];
        stack.pop(e.ptr, e.size);
      }
    }
  }
}
