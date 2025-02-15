#include <gtest/gtest.h>

#include <cstddef>
#include <random>
#include <vector>

#include "libfork/stack.hpp"

void 

test_alloc(std::size_t k_allocs) {
  //
  lf::stack_ptr stack = lf::make_stack();

  // Generate a list of random numbers

  constexpr int low = -100;
  constexpr int high = 200;

  std::vector<int> sizes(k_allocs);

  std::random_device rdev;
  std::mt19937 gen(rdev());
  std::uniform_int_distribution<int> dis(low, high);

  for (std::size_t i = 0; i < k_allocs; i++) {
    sizes[i] = dis(gen);
  }

  std::vector<void *> allocs;

  for (int size : sizes) {
    if (size < 0 && !allocs.empty()) {
      auto *last = allocs.back();
      allocs.pop_back();
      stack.deallocate(last);
    } else if (size > 0) {
      allocs.push_back(stack.allocate(lf::detail::checked_cast<std::size_t>(size)));
    }
  }

  while (!allocs.empty()) {
    auto *last = allocs.back();
    allocs.pop_back();
    stack.deallocate(last);
  }

  EXPECT_TRUE(stack.empty());
}

TEST(Stack, Initialization) {
  lf::stack_ptr stack;
  EXPECT_FALSE(stack);
  EXPECT_TRUE(stack.empty());

  stack = lf::make_stack();
  EXPECT_TRUE(stack);
  EXPECT_TRUE(stack.empty());
}

TEST(Stack, PushPop) {
  for (std::size_t allocs : {0U, 1U, 10U, 100U, 1000U, 10000U, 100000U}) {
    test_alloc(allocs);
  }
}
