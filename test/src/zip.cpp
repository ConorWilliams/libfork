#include <catch2/catch_test_macros.hpp>

#include "libfork/zip_stack.hpp"

TEST_CASE("Basic stack ops", "[stack]") {

  lf::stack::handle h;
  REQUIRE(!h);

  auto *ptr = h.allocate(32);
  REQUIRE(h);

  auto w = h.weak();
  REQUIRE(w == h);
  std::move(h).release();
  REQUIRE(!h);

  auto h2 = std::move(w).acquire();
  h2.deallocate(ptr, 32);
}
