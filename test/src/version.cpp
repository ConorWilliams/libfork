#include <catch2/catch_test_macros.hpp>

#include "libfork/version.hpp"

#ifndef LF_VERSION_MAJOR
  #error LF_VERSION_MAJOR macro is missing
#endif

#ifndef LF_VERSION_MINOR
  #error LF_VERSION_MINOR macro is missing
#endif

#ifndef LF_VERSION_PATCH
  #error LF_VERSION_PATCH macro is missing
#endif

TEST_CASE("Version header", "[version]") {
  constexpr std::size_t major{LF_VERSION_MAJOR};
  constexpr std::size_t minor{LF_VERSION_MINOR};
  constexpr std::size_t patch{LF_VERSION_PATCH};

  REQUIRE(major >= 4);
  REQUIRE(minor >= 0);
  REQUIRE(patch >= 0);
}
