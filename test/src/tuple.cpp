#include <catch2/catch_test_macros.hpp>
#include <type_traits>

import std;
import libfork.core;

namespace {

template <typename T>
struct control_struct {
  T val;
};

struct empty {};

template <typename T>
void check_accessor_types() {
  using tuple_t = lf::tuple<T>;
  using control_t = control_struct<T>;

  // lvalue ref
  STATIC_REQUIRE(std::is_same_v<decltype((std::declval<tuple_t &>().template get<0>())),
                                decltype((std::declval<control_t &>().val))>);

  // const lvalue ref
  STATIC_REQUIRE(std::is_same_v<decltype((std::declval<tuple_t const &>().template get<0>())),
                                decltype((std::declval<control_t const &>().val))>);

  // rvalue ref
  STATIC_REQUIRE(std::is_same_v<decltype((std::declval<tuple_t &&>().template get<0>())),
                                decltype((std::declval<control_t &&>().val))>);

  // const rvalue ref
  STATIC_REQUIRE(std::is_same_v<decltype((std::declval<tuple_t const &&>().template get<0>())),
                                decltype((std::declval<control_t const &&>().val))>);
}

} // namespace

TEST_CASE("Tuple accessor types", "[tuple]") {
  check_accessor_types<int>();
  check_accessor_types<int &>();
  check_accessor_types<int const>();
  check_accessor_types<int const &>();
  check_accessor_types<int &&>();
  check_accessor_types<int const &&>();
}

TEST_CASE("Tuple size optimization", "[tuple]") {
  STATIC_REQUIRE(sizeof(lf::tuple<int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<int, empty>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<empty>) == 1);
  // STATIC_REQUIRE(sizeof(lf::tuple<empty, empty>) == 1);
  STATIC_REQUIRE(sizeof(lf::tuple<empty, int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<empty, empty, int>) == sizeof(int));
  // STATIC_REQUIRE(sizeof(lf::tuple<empty, int, empty>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<int, int>) == 2 * sizeof(int));
}

TEST_CASE("Tuple triviality", "[tuple]") {
  //
  using trivial_tuple = lf::tuple<int, double, empty>;

  STATIC_REQUIRE(std::is_aggregate_v<trivial_tuple>);

  STATIC_REQUIRE(std::is_trivially_default_constructible_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_copy_constructible_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_move_constructible_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_copy_assignable_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_move_assignable_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_destructible_v<trivial_tuple>);
}
