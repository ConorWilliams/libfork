
#include <type_traits>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/ev.hpp"

namespace {

template <typename T>
concept good_deref = requires (lf::ev<T> val, T ref) {
  { *val } -> std::same_as<decltype((ref))>;
};

template <typename T>
struct add_const : std::type_identity<T const> {};

template <typename T>
struct add_const<T &> : std::type_identity<T const &> {};

template <typename T>
struct add_const<T &&> : std::type_identity<T const &&> {};

template <typename T>
concept good_decref = requires (lf::ev<T> const val, typename add_const<T>::type ref) {
  { *val } -> std::same_as<decltype((ref))>;
};

struct nt {
  nt() {}
};

static_assert(!lf::trivial_return<nt>);

} // namespace

TEMPLATE_TEST_CASE("Ev operator *", "[ev]", nt, int, int &, int &&, int const &, int const &&) {

  using U = std::remove_reference_t<TestType>;

  static_assert(good_deref<TestType>);
  static_assert(good_decref<TestType>);

  lf::ev<TestType> val{};
  static_assert(std::same_as<decltype(*val), U &>);
  static_assert(std::same_as<decltype(*std::move(val)), U &&>);

  lf::ev<TestType> const cval{};
  static_assert(std::same_as<decltype(*cval), U const &>);
  static_assert(std::same_as<decltype(*std::move(cval)), U const &&>);
}

TEMPLATE_TEST_CASE("Eventually operator ->", "[ev]", nt, int, int &) {

  using U = std::remove_reference_t<TestType>;

  lf::ev<TestType> val{};
  static_assert(std::same_as<decltype(val.operator->()), U *>);

  lf::ev<TestType> const cval{};
  static_assert(std::same_as<decltype(cval.operator->()), U const *>);
}
