#include <catch2/catch_test_macros.hpp>

import std;

import libfork.core;
import libfork.context;

using namespace lf;

namespace {

struct bad_alloc : dummy_allocator {
  constexpr static auto pop(void *p, std::size_t sz) -> void;
};

struct bad_context : dummy_context {
  constexpr static auto release() -> void;
};

} // namespace

TEST_CASE("Concepts", "[deque]") {
  STATIC_REQUIRE(stack_allocator<dummy_allocator>);
  STATIC_REQUIRE(worker_context<dummy_context>);

  STATIC_REQUIRE_FALSE(stack_allocator<bad_alloc>);
  STATIC_REQUIRE_FALSE(worker_context<bad_context>);
}
