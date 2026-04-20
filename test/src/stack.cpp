#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;
import libfork.utils;

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
  REQUIRE(impl());                                                                                           \
  STATIC_REQUIRE(impl())

namespace {

constexpr void check_alignment(void *ptr) {

  expect(ptr != nullptr);

  if !consteval {
    REQUIRE(lf::is_sufficiently_aligned<k_new_align>(ptr));
  }
}

constexpr void check_empty(auto const &stack) {
  if constexpr (requires { stack.empty(); }) {
    expect(stack.empty());
  }
}

constexpr void check_non_empty(auto const &stack) {
  if constexpr (requires { stack.empty(); }) {
    expect(!stack.empty());
  }
}

} // namespace

// Stack types that may hit slab_stack's fixed capacity need exception support
// to signal overflow. Under -fno-exceptions, drop slab_stack from those tests.
#if LF_COMPILER_EXCEPTIONS
  #define STACK_TYPES_ALL lf::geometric_stack<>, lf::adaptor_stack<>, lf::slab_stack<>
#else
  #define STACK_TYPES_ALL lf::geometric_stack<>, lf::adaptor_stack<>
#endif

TEMPLATE_TEST_CASE("Concept", "[stacks]", lf::geometric_stack<>, lf::adaptor_stack<>, lf::slab_stack<>) {
  STATIC_REQUIRE(worker_stack<TestType>); //
}

TEMPLATE_TEST_CASE("Basic push and pop", "[stacks]", lf::geometric_stack<>, lf::adaptor_stack<>,
                   lf::slab_stack<>) {
  TEST_CONSTEXPR([]() -> bool {
    TestType stack;
    check_empty(stack);

    void *p1 = stack.push(10);
    check_alignment(p1);
    check_non_empty(stack);

    void *p2 = stack.push(20);
    check_alignment(p2);
    expect(p2 != p1);
    check_non_empty(stack);

    // Pop in FILO order
    stack.pop(p2, 20);
    stack.pop(p1, 10);
    check_empty(stack);

    return true;
  });
}

TEMPLATE_TEST_CASE("Checkpoint and Acquire/Release", "[stacks]", lf::geometric_stack<>, lf::adaptor_stack<>,
                   lf::slab_stack<>) {
  TEST_CONSTEXPR([]() -> bool {
    TestType stack1;
    void *p1 = stack1.push(100);
    auto cp1 = stack1.checkpoint();

    TestType stack2;
    auto cp2 = stack2.checkpoint();

    using C = decltype(cp1);

    expect(((cp1 == C{} && cp2 == C{}) || cp1 != cp2));

    auto key1 = stack1.prepare_release();
    stack2.acquire(cp1);
    stack1.release(key1);
    expect(stack2.checkpoint() == cp1);
    stack2.pop(p1, 100);

    return true;
  });
}

TEMPLATE_TEST_CASE("Single pass", "[stacks]", STACK_TYPES_ALL) {
  for (int k = 0; k < 10; ++k) {

    TestType stack;
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

      // Push phase — break early if slab_stack exhausts its fixed capacity
      for (std::size_t j = 0; j < depth; ++j) {
        std::size_t s = size_dist(rng);
        void *p = nullptr;
#if LF_COMPILER_EXCEPTIONS
        try {
          p = stack.push(s);
        } catch (std::bad_alloc const &) {
          break;
        }
#else
        p = stack.push(s);
#endif
        check_alignment(p);
        entries.push_back({.ptr = p, .size = s});
      }

      // Pop phase (FILO) — use entries.size() in case push exited early
      for (std::size_t j = entries.size(); j > 0; --j) {
        auto const &e = entries[j - 1];
        stack.pop(e.ptr, e.size);
      }

      check_empty(stack);
    }
  }
}

TEMPLATE_TEST_CASE("Randomized push/pop", "[stacks]", STACK_TYPES_ALL) {
  TestType stack;
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
      check_empty(stack);
    }

    if (entries.empty() || push_dist(rng)) {
      std::size_t s = size_dist(rng);
      void *p = nullptr;
#if LF_COMPILER_EXCEPTIONS
      try {
        p = stack.push(s);
      } catch (std::bad_alloc const &) {
        break; // slab_stack exhausted; clean up and finish
      }
#else
      p = stack.push(s);
#endif
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

  check_empty(stack);
}

TEMPLATE_TEST_CASE("Spikey randomized push/pop", "[stacks]", STACK_TYPES_ALL) {
  TestType stack;
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
      check_empty(stack);
      do_push = true;
    } else if (last_was_push) {
      do_push = push_after_push(rng);
    } else {
      do_push = push_after_pop(rng);
    }

    if (do_push) {
      std::size_t s = size_dist(rng);
      void *p = nullptr;
#if LF_COMPILER_EXCEPTIONS
      try {
        p = stack.push(s);
      } catch (std::bad_alloc const &) {
        break; // slab_stack exhausted; clean up and finish
      }
#else
      p = stack.push(s);
#endif
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
  check_empty(stack);
}

// ---- slab_stack specific ----
//
// Tests that exercise behaviour unique to slab_stack's fixed-size design.

#if LF_COMPILER_EXCEPTIONS
TEST_CASE("slab_stack - throws when full", "[stacks]") {
  // Use a tiny slab (2 usable nodes) to exercise the overflow path precisely.
  lf::slab_stack<> stack(2);

  void *p1 = stack.push(k_new_align);
  void *p2 = stack.push(k_new_align);
  REQUIRE_THROWS_AS(stack.push(k_new_align), std::bad_alloc);

  stack.pop(p2, k_new_align);
  stack.pop(p1, k_new_align);
  check_empty(stack);
}
#endif

TEST_CASE("slab_stack - single pass", "[stacks]") {
  for (int k = 0; k < 10; ++k) {
    // Slab sized to hold the worst-case live footprint without early exit:
    // depth_max (5000) * roundup(size_max (200), k_new_align=16) / k_new_align
    // = 5000 * 13 = 65 000 nodes, with headroom.
    lf::slab_stack<> stack(70'000);
    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> size_dist{1, 200};
    std::uniform_int_distribution<std::size_t> depth_dist{5, 5000};

    struct entry {
      void *ptr;
      std::size_t size;
    };

    for (int i = 0; i < 2; ++i) {
      std::vector<entry> entries;
      const std::size_t depth = depth_dist(rng);

      for (std::size_t j = 0; j < depth; ++j) {
        std::size_t s = size_dist(rng);
        void *p = stack.push(s);
        check_alignment(p);
        entries.push_back({.ptr = p, .size = s});
      }

      for (std::size_t j = depth; j > 0; --j) {
        auto const &e = entries[j - 1];
        stack.pop(e.ptr, e.size);
      }

      check_empty(stack);
    }
  }
}

#if LF_COMPILER_EXCEPTIONS

TEST_CASE("slab_stack - release/acquire preserves capacity", "[stacks]") {
  // Regression: acquire must propagate the non-default capacity via m_ctrl->size,
  // not silently revert to k_default_nodes.
  constexpr int N = 4;
  lf::slab_stack<> src(N);
  lf::slab_stack<> dst;

  void *p = src.push(k_new_align);
  auto cp = src.checkpoint();
  auto key = src.prepare_release();
  dst.acquire(cp);
  src.release(key);

  // dst should have room for N-1 more pushes (one already used), and then throw.
  std::vector<void *> ptrs{p};
  for (int i = 1; i < N; ++i) {
    ptrs.push_back(dst.push(k_new_align));
  }
  REQUIRE_THROWS_AS(dst.push(k_new_align), std::bad_alloc);
  for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it) {
    dst.pop(*it, k_new_align);
  }
  REQUIRE(dst.empty());
}

#endif
