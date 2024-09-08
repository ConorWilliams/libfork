#include <string>

#include "libfork/libfork.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Name is libfork", "[library]")
{
  auto const exported = exported_class {};
  REQUIRE(std::string("libfork") == exported.name());
}
