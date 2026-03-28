#include <catch2/catch_test_macros.hpp>

import std;
import libfork.core;

using lf::geometric_stack;
using lf::stack;
using lf::stack_allocator;

// TEST_CASE("Stack properties", "[stack]") { STATIC_REQUIRE(stack_allocator<stack>); }

TEST_CASE("Stack allocation", "[stack]") {
  stack s;

  geometric_stack gs;
  gs.checkpoint() == gs.checkpoint();
  gs.release();

  void *p1 = s.push(10);
  REQUIRE(p1 != nullptr);

  void *p2 = s.push(20);
  REQUIRE(p2 != nullptr);
  REQUIRE(p2 != p1);

  s.pop(p2, 20);
  s.pop(p1, 10);
}

TEST_CASE("Stack checkpoint/release/acquire", "[stack]") {
  stack s1;
  void *p1 = s1.push(10);

  auto cp = s1.checkpoint();
  s1.release();

  // s1 is now empty (initial stacklet)
  // p1 is still valid in the detached stack

  stack s2;
  s2.acquire(cp);

  // s2 now owns the stack with p1
  s2.pop(p1, 10);
}

TEST_CASE("Stack growth", "[stack]") {
  stack s;
  std::vector<void *> ptrs;
  // Allocate a lot to force new stacklets
  for (std::size_t i = 0; i < 1000; ++i) {
    ptrs.push_back(s.push(1024));
  }

  for (std::size_t i = 999; i < 1000; --i) {
    s.pop(ptrs[i], 1024);
  }
}
