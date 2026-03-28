#include <catch2/catch_test_macros.hpp>

import std;
import libfork.core;

using namespace lf;

TEST_CASE("Stack: Concepts", "[stack]") {
  STATIC_REQUIRE(stack_allocator<geometric_stack>);
}

TEST_CASE("Stack: Basic push and pop", "[stack]") {
  geometric_stack stack;

  void* p1 = stack.push(10);
  REQUIRE(p1 != nullptr);
  // Check alignment (k_new_align is exported from constants)
  REQUIRE((reinterpret_cast<std::uintptr_t>(p1) % k_new_align) == 0);

  void* p2 = stack.push(20);
  REQUIRE(p2 != nullptr);
  REQUIRE(p2 != p1);
  REQUIRE((reinterpret_cast<std::uintptr_t>(p2) % k_new_align) == 0);

  // Pop in FILO order
  stack.pop(p2, 20);
  stack.pop(p1, 10);
}

TEST_CASE("Stack: Large allocation", "[stack]") {
  geometric_stack stack;
  // A very large allocation that definitely won't fit in initial stacklet (if any)
  void* p = stack.push(1024 * 1024);
  REQUIRE(p != nullptr);
  REQUIRE((reinterpret_cast<std::uintptr_t>(p) % k_new_align) == 0);
  stack.pop(p, 1024 * 1024);
}

TEST_CASE("Stack: Geometric growth", "[stack]") {
  geometric_stack stack;
  std::vector<void*> ptrs;
  std::size_t size = 16;
  for (std::size_t i = 0; i < 20; ++i) {
    void* p = stack.push(size);
    REQUIRE(p != nullptr);
    REQUIRE((reinterpret_cast<std::uintptr_t>(p) % k_new_align) == 0);
    ptrs.push_back(p);
    size *= 2;
  }

  for (std::size_t i = 19; i < 20; --i) {
    size /= 2;
    stack.pop(ptrs[i], size);
  }
}

TEST_CASE("Stack: Caching", "[stack]") {
  geometric_stack stack;
  void* p1 = stack.push(100);
  stack.pop(p1, 100);

  // Next push should ideally reuse the cached stacklet
  void* p2 = stack.push(100);
  REQUIRE(p2 != nullptr);
  // It's hard to verify it's the SAME stacklet without internal access,
  // but we can at least verify basic functionality.
  stack.pop(p2, 100);
}

TEST_CASE("Stack: Checkpoint and Acquire/Release", "[stack]") {
  geometric_stack stack1;
  void* p1 = stack1.push(100);
  auto cp1 = stack1.checkpoint();

  geometric_stack stack2;
  auto cp2 = stack2.checkpoint();

  REQUIRE(cp1 != cp2);

  SECTION("Acquire other stack") {
    auto key1 = stack1.prepare_release();
    stack1.release(key1);
    
    stack2.acquire(cp1);

    REQUIRE(stack2.checkpoint() == cp1);
    
    // We should be able to pop p1 from stack2 now!
    stack2.pop(p1, 100);
  }

  SECTION("Acquire default checkpoint") {
    geometric_stack stack3;
    geometric_stack::checkpoint_t cp_default;
    // Should be a no-op and not crash
    stack3.acquire(cp_default);
    REQUIRE(stack3.checkpoint() != cp_default);
  }
}
