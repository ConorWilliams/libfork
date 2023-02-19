#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN No linting in tests

int factorial(int number) {
  return number <= 1 ? number : factorial(number - 1) * number;
}

TEST_CASE("Factorials", "[catch]") {
  REQUIRE(factorial(1) == 1);
  REQUIRE(factorial(2) == 2);
  REQUIRE(factorial(3) == 6);
  REQUIRE(factorial(10) == 3'628'800);
}

// NOLINTEND