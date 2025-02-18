#include <memory>

#include <catch2/catch_test_macros.hpp>

namespace {

consteval auto cons() -> bool {
  int x = 4;
  std::construct_at(&x, 3);
  return x == 3;
}

} // namespace

TEST_CASE("Test construct", "[constexpr]") { REQUIRE(cons()); }
