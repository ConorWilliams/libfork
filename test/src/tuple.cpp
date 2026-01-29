#include <type_traits>

#include <catch2/catch_test_macros.hpp>

import std;

import libfork.core;

namespace {

template <typename T>
struct control_struct {
  T val;
};

struct nil {};

template <typename T>
using get = decltype(std::declval<T>().template get<0>());

template <typename T>
using val = decltype((std::declval<T>().val));

template <typename T>
void check_accessor_types() {
  using tupl_t = lf::tuple<T>;
  using ctrl_t = control_struct<T>;

  STATIC_REQUIRE(std::same_as<get<tupl_t &>, val<ctrl_t &>>);
  STATIC_REQUIRE(std::same_as<get<tupl_t const &>, val<ctrl_t const &>>);
  STATIC_REQUIRE(std::same_as<get<tupl_t &&>, val<ctrl_t &&>>);
  STATIC_REQUIRE(std::same_as<get<tupl_t const &&>, val<ctrl_t const &&>>);
}

} // namespace

TEST_CASE("Tuple accessor types", "[tuple]") {
  check_accessor_types<int>();
  check_accessor_types<int &>();
  check_accessor_types<int &&>();

  check_accessor_types<int const>();
  check_accessor_types<int const &>();
  check_accessor_types<int const &&>();
}

TEST_CASE("Tuple size optimization", "[tuple]") {

  STATIC_REQUIRE(sizeof(lf::tuple<int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<nil>) == 1);

  STATIC_REQUIRE(sizeof(lf::tuple<int, nil>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<nil, int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<int, nil>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<int, int>) == 2 * sizeof(int));

  STATIC_REQUIRE(sizeof(lf::tuple<nil, nil, int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<nil, nil, int>) == sizeof(int));
  // STATIC_REQUIRE(sizeof(lf::tuple<empty, int, empty>) == sizeof(int));
}

TEST_CASE("Tuple triviality", "[tuple]") {
  //
  using trivial_tuple = lf::tuple<int, double, nil>;

  STATIC_REQUIRE(std::is_aggregate_v<trivial_tuple>);

  STATIC_REQUIRE(std::is_trivially_default_constructible_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_copy_constructible_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_move_constructible_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_copy_assignable_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_move_assignable_v<trivial_tuple>);
  STATIC_REQUIRE(std::is_trivially_destructible_v<trivial_tuple>);
}

TEST_CASE("Tuple construction", "[tuple]") {
  lf::tuple<int, double> _{1, 0.};
  lf::tuple<nil, int, nil> _{nil{}, 2, nil{}};
}
