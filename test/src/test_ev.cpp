
#include <memory>
#include <type_traits>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/ev.hpp"

namespace {

template <typename T, typename Ref>
concept deref_like = requires (T val, Ref ref) {
  { *val } -> std::same_as<decltype((ref))>;
  { *std::move(val) } -> std::same_as<decltype(std::move(ref))>;
};

struct non_trivial {
  constexpr ~non_trivial() {}
};

static_assert(!lf::trivial_return<non_trivial>);

consteval auto const_test() -> bool {

  {
    lf::ev<non_trivial const> x;
    x.emplace();
  }

  {
    lf::ev<int> x;
    int const *p = x.get();
    std::construct_at(p, 3);
  }

  return true;
}

} // namespace

TEST_CASE("Ev constexpr tests", "[ev]") { REQUIRE(const_test()); }

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

TEMPLATE_TEST_CASE("Eventually operator ->", "[ev]", non_trivial, non_trivial &, int, int &) {

  using deref = std::remove_reference_t<TestType>;

  lf::ev<TestType> val{};
  static_assert(std::same_as<decltype(val.operator->()), deref *>);

  lf::ev<TestType> const cval{};
  static_assert(std::same_as<decltype(cval.operator->()), deref const *>);
}
