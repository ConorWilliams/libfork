#include <catch2/catch_test_macros.hpp>

import std;

import libfork;

using namespace lf;

TEST_CASE("Constructible", "[adaptors]") {
  // adapt_vector's default ctor only stores the allocator; no allocation occurs,
  // so it should be noexcept when the allocator's default ctor is noexcept.
  STATIC_REQUIRE(std::is_nothrow_default_constructible_v<adapt_vector<>>);

  // adapt_deque's default ctor delegates to a capacity-taking ctor that
  // allocates an internal buffer, so it must NOT be noexcept — bad_alloc must
  // propagate instead of invoking std::terminate.
  STATIC_REQUIRE(!std::is_nothrow_default_constructible_v<adapt_deque<>>);
}

TEST_CASE("adapt_deque: default constructor allocates", "[adaptors]") {
  adapt_deque<> d;
  // Freshly constructed: steal returns an empty handle.
  REQUIRE_FALSE(d.steal());
  REQUIRE_FALSE(d.pop());
}

TEST_CASE("adapt_vector: default constructor does not allocate", "[adaptors]") {
  adapt_vector<> v;
  REQUIRE_FALSE(v.pop());
}

namespace {

using test_stack = geometric_stack<>;
using test_deque = adapt_deque<>;

TEST_CASE("derived_poly_context: piecewise construction compiles", "[contexts]") {
  derived_poly_context<test_stack, test_deque> ctx{
      std::piecewise_construct,
      std::tuple<>{},
      std::tuple<std::size_t>{1024},
  };

  std::ignore = ctx;
}

TEST_CASE("derived_poly_context: piecewise construction forwards stack args", "[contexts]") {
  derived_poly_context<test_stack, test_deque> ctx{
      std::piecewise_construct,
      std::tuple<std::allocator<std::byte>>{},
      std::tuple<std::size_t>{1024},
  };

  std::ignore = ctx;
}

TEST_CASE("mono_context: piecewise construction compiles", "[contexts]") {
  mono_context<test_stack, test_deque> ctx{
      std::piecewise_construct,
      std::tuple<>{},
      std::tuple<std::size_t>{1024},
  };

  std::ignore = ctx;
}

TEST_CASE("mono_context: piecewise construction forwards stack args", "[contexts]") {
  mono_context<test_stack, test_deque> ctx{
      std::piecewise_construct,
      std::tuple<std::allocator<std::byte>>{},
      std::tuple<std::size_t>{1024},
  };

  std::ignore = ctx;
}

} // namespace
