
#include <memory>
#include <type_traits>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/ev.hpp"

namespace {

struct non_trivial {
  int x;
  constexpr ~non_trivial() {}
};

static_assert(!lf::trivial_return<non_trivial>);

consteval auto const_test() -> bool {

  {
    lf::ev<non_trivial const> x;
    x.emplace(5);
  }

  {
    lf::ev<int const> x;
    int const *p = x.get();
    std::construct_at(p, 3);
  }

  return true;
}

} // namespace

TEST_CASE("Ev constexpr tests", "[ev]") { REQUIRE(const_test()); }

namespace {

template <typename T, typename Ref>
concept deref_like = requires (T val, Ref ref) {
  { *val } -> std::same_as<decltype((ref))>;
  { *std::move(val) } -> std::same_as<decltype(std::move(ref))>;
};

} // namespace

TEMPLATE_TEST_CASE("Ev operator *", "[ev]", non_trivial, int) {

  using lf::ev;

  static_assert(deref_like<ev<TestType>, TestType>);
  static_assert(deref_like<ev<TestType> const, TestType const>);

  static_assert(deref_like<ev<TestType const>, TestType const>);
  static_assert(deref_like<ev<TestType const> const, TestType const>);

  static_assert(deref_like<ev<TestType &>, TestType &>);
  static_assert(deref_like<ev<TestType &> const, TestType const &>);

  static_assert(deref_like<ev<TestType const &>, TestType const &>);
  static_assert(deref_like<ev<TestType const &> const, TestType const &>);

  static_assert(deref_like<ev<TestType &&>, TestType &&>);
  static_assert(deref_like<ev<TestType &&> const, TestType const &&>);

  static_assert(deref_like<ev<TestType const &&>, TestType const &&>);
  static_assert(deref_like<ev<TestType const &&> const, TestType const &&>);
}

namespace {

template <typename T, typename Ptr>
concept points_like = requires (T val) { requires std::same_as<decltype(val.operator->()), Ptr>; };

} // namespace

TEMPLATE_TEST_CASE("Eventually operator ->", "[ev]", non_trivial, int) {

  using lf::ev;

  static_assert(points_like<ev<TestType>, TestType *>);
  static_assert(points_like<ev<TestType> const, TestType const *>);

  static_assert(points_like<ev<TestType const>, TestType const *>);
  static_assert(points_like<ev<TestType const> const, TestType const *>);

  static_assert(points_like<ev<TestType &>, TestType *>);
  static_assert(points_like<ev<TestType &> const, TestType const *>);

  static_assert(points_like<ev<TestType const &>, TestType const *>);
  static_assert(points_like<ev<TestType const &> const, TestType const *>);

  // TODO: &&
}
