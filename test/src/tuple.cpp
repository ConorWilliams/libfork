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
using val = decltype(std::get<0>(std::declval<T>()));

struct any {
  template <typename T>
  constexpr operator T() const noexcept {}
};

template <typename T>
void check_accessor_types() {
  using tupl_t = lf::tuple<T>;
  using ctrl_t = std::tuple<T>;

  STATIC_REQUIRE(std::same_as<get<tupl_t &>, val<ctrl_t &>>);
  STATIC_REQUIRE(std::same_as<get<tupl_t const &>, val<ctrl_t const &>>);
  STATIC_REQUIRE(std::same_as<get<tupl_t &&>, val<ctrl_t &&>>);
  STATIC_REQUIRE(std::same_as<get<tupl_t const &&>, val<ctrl_t const &&>>);

  int val = 0;

  // Force instantiation
  tupl_t t{static_cast<T>(val)};

  std::ignore = static_cast<tupl_t &>(t).get<0>();
  std::ignore = static_cast<tupl_t const &>(t).get<0>();
  std::ignore = static_cast<tupl_t &&>(t).get<0>();
  std::ignore = static_cast<tupl_t const &&>(t).get<0>();
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

  STATIC_REQUIRE(sizeof(lf::tuple<nil>) == 1);
  STATIC_REQUIRE(sizeof(lf::tuple<int>) == sizeof(int));

  STATIC_REQUIRE(sizeof(lf::tuple<int, nil>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<nil, int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<nil, nil>) == sizeof(std::tuple<nil, nil>));
  STATIC_REQUIRE(sizeof(lf::tuple<int, int>) == 2 * sizeof(int));

  STATIC_REQUIRE(sizeof(lf::tuple<nil, nil, int>) == sizeof(int));
  STATIC_REQUIRE(sizeof(lf::tuple<int, nil, nil>) == 2 * sizeof(int)); // TODO: fixable?
  STATIC_REQUIRE(sizeof(lf::tuple<nil, int, nil>) == 2 * sizeof(int));
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

TEST_CASE("Tuple apply", "[tuple]") {

  lf::tuple<int, lf::move_only> val{1, lf::move_only{}};

  REQUIRE(std::move(val).apply([](int x, lf::move_only) -> bool {
    return x == 1;
  }));
}

TEST_CASE("Tuple structured bindings", "[tuple]") {

  lf::tuple<int, nil> tup{1, nil{}};

  auto &&[i, n] = tup;

  REQUIRE(i == 1);
  REQUIRE(std::is_same_v<decltype(n), nil>);

  i += 1;

  REQUIRE(tup.get<0>() == 2);
}

TEST_CASE("Tuple CTAD", "[tuple]") {
  int x = 42;
  lf::tuple t_lval{x};
  STATIC_REQUIRE(std::is_same_v<decltype(t_lval), lf::tuple<int &>>);

  lf::tuple t_rval{42};
  STATIC_REQUIRE(std::is_same_v<decltype(t_rval), lf::tuple<int>>);

  const int cx = 42;
  lf::tuple t_clval{cx};
  STATIC_REQUIRE(std::is_same_v<decltype(t_clval), lf::tuple<const int &>>);
}

TEST_CASE("Tuple reference semantics", "[tuple]") {
  int x = 1;
  lf::tuple<int &> t{x};
  t.get<0>() = 2;
  REQUIRE(x == 2);

  auto &[ref] = t;
  ref = 3;
  REQUIRE(x == 3);
}

TEST_CASE("Tuple rvalue reference semantics", "[tuple]") {
  int x = 1;
  // tuple holding rvalue ref to x (cast to rvalue)
  lf::tuple<int &&> t{std::move(x)};

  // Accessing lvalue tuple -> lvalue ref to member (which is rvalue ref) -> int&
  STATIC_REQUIRE(std::is_same_v<decltype(t.get<0>()), int &>);

  // Modifying via the stored rvalue ref
  t.get<0>() = 2;
  REQUIRE(x == 2);

  // Accessing rvalue tuple -> xvalue member -> int&&
  STATIC_REQUIRE(std::is_same_v<decltype(std::move(t).get<0>()), int &&>);
}

TEST_CASE("Tuple apply value categories", "[tuple]") {
  lf::tuple<int> t{42};

  // Check lvalue arg
  bool called_lvalue = false;
  t.apply([&](int &x) {
    called_lvalue = true;
    REQUIRE(x == 42);
  });
  REQUIRE(called_lvalue);

  // Check rvalue arg
  bool called_rvalue = false;
  std::move(t).apply([&](int &&x) {
    called_rvalue = true;
    REQUIRE(x == 42);
  });
  REQUIRE(called_rvalue);

  // Check const lvalue arg
  const lf::tuple<int> ct{42};
  bool called_const_lvalue = false;
  ct.apply([&](const int &x) {
    called_const_lvalue = true;
    REQUIRE(x == 42);
  });
  REQUIRE(called_const_lvalue);
}

TEST_CASE("Tuple empty", "[tuple]") {
  lf::tuple<> t{};
  STATIC_REQUIRE(sizeof(t) == 1);
  STATIC_REQUIRE(std::tuple_size_v<decltype(t)> == 0);
}

TEST_CASE("Tuple move-only types", "[tuple]") {
  auto ptr = std::make_unique<int>(42);
  lf::tuple<std::unique_ptr<int>> t{std::move(ptr)};

  REQUIRE(ptr == nullptr);
  REQUIRE(*t.get<0>() == 42);

  // Move out
  auto ptr2 = std::move(t.get<0>());
  REQUIRE(*ptr2 == 42);
  REQUIRE(t.get<0>() == nullptr);
}

TEST_CASE("Tuple nested", "[tuple]") {
  lf::tuple<lf::tuple<int, int>, int> t{1, 2, 3};

  REQUIRE(t.get<0>().get<0>() == 1);
  REQUIRE(t.get<0>().get<1>() == 2);
  REQUIRE(t.get<1>() == 3);
}

TEST_CASE("Tuple const structured bindings", "[tuple]") {
  lf::tuple<int, int> t{10, 20};
  const auto &[x, y] = t;

  STATIC_REQUIRE(std::is_same_v<decltype(x), const int>);
  STATIC_REQUIRE(std::is_same_v<decltype(y), const int>);

  t.get<0>() = 0;
  t.get<1>() = 1;

  REQUIRE(x == 0);
  REQUIRE(y == 1);
}

TEST_CASE("Tuple traits", "[tuple]") {
  using T = lf::tuple<int, double, char>;
  STATIC_REQUIRE(std::tuple_size_v<T> == 3);
  STATIC_REQUIRE(std::is_same_v<std::tuple_element_t<0, T>, int>);
  STATIC_REQUIRE(std::is_same_v<std::tuple_element_t<1, T>, double>);
  STATIC_REQUIRE(std::is_same_v<std::tuple_element_t<2, T>, char>);
}
