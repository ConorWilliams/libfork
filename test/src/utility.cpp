#include <catch2/catch_test_macros.hpp>

import std;

import libfork;
import libfork.utils;

TEST_CASE("Defer properties", "[defer]") {
  using fn_t = void (*)() noexcept;
  STATIC_REQUIRE(!std::is_copy_constructible_v<lf::defer<fn_t>>);
  STATIC_REQUIRE(!std::is_move_constructible_v<lf::defer<fn_t>>);
  STATIC_REQUIRE(!std::is_copy_assignable_v<lf::defer<fn_t>>);
  STATIC_REQUIRE(!std::is_move_assignable_v<lf::defer<fn_t>>);
}

TEST_CASE("Defer executes on scope exit", "[defer]") {
  int count = 0;
  {
    lf::defer _ = [&count]() noexcept -> void {
      ++count;
    };
    REQUIRE(count == 0);
  }
  REQUIRE(count == 1);
}
