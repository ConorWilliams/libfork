#include <catch2/catch_test_macros.hpp>

#include "libfork/zip_stack.hpp"

TEST_CASE("Basic stack ops", "[stack]") {

  lf::stack::handle h;

  REQUIRE(!bool(h));

  auto *ptr = h.allocate(32);
}
