#include <catch2/catch_test_macros.hpp>

import std;

import libfork.core;

using lf::immovable;
using lf::move_only;

namespace {

struct test_immovable : immovable {};
struct test_move_only : move_only {};

} // namespace

TEST_CASE("Utility properties", "[utility]") {

  STATIC_REQUIRE(std::is_default_constructible_v<immovable>);
  STATIC_REQUIRE(!std::is_copy_constructible_v<immovable>);
  STATIC_REQUIRE(!std::is_move_constructible_v<immovable>);
  STATIC_REQUIRE(!std::is_copy_assignable_v<immovable>);
  STATIC_REQUIRE(!std::is_move_assignable_v<immovable>);

  STATIC_REQUIRE(std::is_default_constructible_v<move_only>);
  STATIC_REQUIRE(!std::is_copy_constructible_v<move_only>);
  STATIC_REQUIRE(std::is_move_constructible_v<move_only>);
  STATIC_REQUIRE(!std::is_copy_assignable_v<move_only>);
  STATIC_REQUIRE(std::is_move_assignable_v<move_only>);
  STATIC_REQUIRE(std::movable<move_only>);

  STATIC_REQUIRE(std::is_default_constructible_v<test_immovable>);
  STATIC_REQUIRE(!std::is_copy_constructible_v<test_immovable>);
  STATIC_REQUIRE(!std::is_move_constructible_v<test_immovable>);

  STATIC_REQUIRE(std::is_default_constructible_v<test_move_only>);
  STATIC_REQUIRE(!std::is_copy_constructible_v<test_move_only>);
  STATIC_REQUIRE(std::is_move_constructible_v<test_move_only>);
  STATIC_REQUIRE(std::is_move_assignable_v<test_move_only>);
  STATIC_REQUIRE(std::movable<test_move_only>);
}
