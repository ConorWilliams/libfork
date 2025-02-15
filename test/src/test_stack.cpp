
#include <print>
#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "libfork/stack.hpp"

TEST_CASE("Basic stack ops", "[stack]") {

  lf::stack::handle h;
  REQUIRE(!h);

  std::size_t k_size = GENERATE(1z, 10'000z);

  auto *ptr = h.allocate(k_size);
  REQUIRE(h);

  auto w = h.weak();
  REQUIRE(w == h);
  std::move(h).release();
  REQUIRE(!h);

  auto h2 = std::move(w).acquire();
  h2.deallocate(ptr, k_size);
}

namespace {

struct alloc {
  void *ptr;
  std::size_t size;
};

} // namespace

TEST_CASE("Multiple allocs", "[stack]") {

  lf::stack::handle h;

  constexpr std::size_t k_size = 3200;

  std::size_t k_allocs = GENERATE(1z, 2z, 10z, 100z, 1000z);

  std::println("k_allocs={}", k_allocs);

  std::vector<alloc> allocs;

  for (std::size_t i = 0; i < k_allocs; i++) {
    auto *ptr = h.allocate(k_size);
    allocs.emplace_back(ptr, k_size);
  }

  while (!allocs.empty()) {
    auto [ptr, size] = allocs.back();
    allocs.pop_back();
    h.deallocate(ptr, size);
  }
}

namespace {

void random_allocations(std::size_t k_allocs);

} // namespace

TEST_CASE("Random allocations", "[stack]") {
  for (std::size_t allocs : {0U, 1U, 10U, 100U, 1000U, 10000U, 100000U}) {
    random_allocations(allocs);
  }
}

namespace {

void random_allocations(std::size_t k_allocs) {
  //
  using lf::stack;

  stack::handle stack = {};

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

  std::vector<alloc> allocs;

  for (int size : sizes) {
    std::println("size={}", size);
    if (size < 0 && !allocs.empty()) {
      auto [ptr, size] = allocs.back();
      allocs.pop_back();
      stack.deallocate(ptr, size);
    } else if (size > 0) {
      void *ptr = stack.allocate(size);
      allocs.emplace_back(ptr, size);
    }
  }

  while (!allocs.empty()) {
    auto [ptr, size] = allocs.back();
    allocs.pop_back();
    stack.deallocate(ptr, size);
  }
}

} // namespace
