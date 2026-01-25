#include <catch2/catch_test_macros.hpp>

import std;
import libfork.core;

using namespace lf;

// immovable properties
static_assert(std::is_default_constructible_v<immovable>);
static_assert(!std::is_copy_constructible_v<immovable>);
static_assert(!std::is_move_constructible_v<immovable>);
static_assert(!std::is_copy_assignable_v<immovable>);
static_assert(!std::is_move_assignable_v<immovable>);

// move_only properties
static_assert(std::is_default_constructible_v<move_only>);
static_assert(!std::is_copy_constructible_v<move_only>);
static_assert(std::is_move_constructible_v<move_only>);
static_assert(!std::is_copy_assignable_v<move_only>);
static_assert(std::is_move_assignable_v<move_only>);

// Mixin tests
namespace {
struct test_immovable : immovable {};
static_assert(std::is_default_constructible_v<test_immovable>);
static_assert(!std::is_copy_constructible_v<test_immovable>);
static_assert(!std::is_move_constructible_v<test_immovable>);

struct test_move_only : move_only {};
static_assert(std::is_default_constructible_v<test_move_only>);
static_assert(!std::is_copy_constructible_v<test_move_only>);
static_assert(std::is_move_constructible_v<test_move_only>);
static_assert(std::is_move_assignable_v<test_move_only>);
} // namespace

TEST_CASE("Utility properties", "[utility]") { SUCCEED("static_asserts passed"); }
