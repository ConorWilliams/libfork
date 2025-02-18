
#include <string>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/ev.hpp"

TEMPLATE_TEST_CASE("Eventually basics", "[ev]", int, std::string) {

  lf::ev<TestType> val{};
  static_assert(std::same_as<decltype(*val), TestType &>);
  static_assert(std::same_as<decltype(*std::move(val)), TestType &&>);

  lf::ev<TestType> const cval{};
  static_assert(std::same_as<decltype(*cval), TestType const &>);
  static_assert(std::same_as<decltype(*std::move(cval)), TestType const &&>);
}
