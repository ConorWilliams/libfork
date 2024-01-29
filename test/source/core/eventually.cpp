// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for INTERNAL_CATCH_NOINTERNAL_CATCH_DEF
#include <exception>                             // for rethrow_exception
#include <initializer_list>                      // for initializer_list
#include <stdexcept>                             // for runtime_error
#include <type_traits>                           // for remove_reference_t
#include <utility>                               // for move
#include <vector>                                // for operator==, vector

#include "libfork/core.hpp"     // for sync_wait, task, try_eventually, stash_...
#include "libfork/schedule.hpp" // for unit_pool

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

// To fully test the eventually class we need to test with
//
// Kinds of types:
// - trivial types
// - non-trivial types
// - l-value reference types
// - r-value reference types
// - void
//
// Operations:
// - has exception
// - no exception

namespace {

template <typename T, bool Exception>
void test_value() {
  eventually<T> even;

  REQUIRE(!even.has_value());

  std::remove_reference_t<T> val = {};
  std::remove_reference_t<T> copy = val;

  if constexpr (std::is_rvalue_reference_v<T>) {
    even = std::move(copy);
  } else {
    even = copy;
  }

  REQUIRE(even.has_value());
  REQUIRE(*even == val);
}

} // namespace

TEMPLATE_TEST_CASE(
    "Eventually: no exception", "[eventually][template]", int, std::vector<int>, int &, int &&) {
  test_value<TestType, false>();
  test_value<TestType, true>();
}

#if LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE(
    "Eventually: exception", "[eventually][template]", int, std::vector<int>, int &, int &&, void) {

  try_eventually<TestType> even;

  REQUIRE(!even.has_exception());

  // clang-format off

  LF_TRY {
    LF_THROW(std::runtime_error("test"));
  } LF_CATCH_ALL {
    stash_exception(even);
  }

  REQUIRE(even.has_exception());

  LF_TRY {
    std::rethrow_exception(std::move(even).exception());
    REQUIRE(false);
  } LF_CATCH_ALL {
    REQUIRE(true);
  }

  // clang-format on
}

#endif

namespace {

template <typename T>
inline constexpr auto fwd = [](auto, T val, bool exception) -> task<T> {
  //
  if (exception) {
    LF_THROW(std::runtime_error("throw requested"));
  }

  if constexpr (std::is_reference_v<T>) {
    if constexpr (std::is_rvalue_reference_v<T>) {
      co_return std::move(val);
    } else {
      co_return val;
    }
  } else {
    co_return std::move(val);
  }
};

template <typename T, bool Exception>
inline auto test_fwd = [](auto, bool throw_something) -> task<void> {
  //
  std::remove_reference_t<T> val = {};
  std::remove_reference_t<T> copy = val;

  basic_eventually<T, Exception> even;

  REQUIRE(even.empty());

  co_await lf::call(&even, fwd<T>)(static_cast<T>(copy), throw_something);

  // clang-format off

  LF_TRY {
    co_await lf::join;
    REQUIRE((!throw_something || Exception));
  } LF_CATCH_ALL {
    REQUIRE((throw_something && !Exception));
  }

  // clang-format on

  if (throw_something) {
    if constexpr (Exception) {
      REQUIRE(even.has_exception());
    }
  } else {
    REQUIRE(!even.empty());
    REQUIRE(even.has_value());
    REQUIRE(*even == val);
  }
};

constexpr auto test_void = [](auto, bool exception) -> task<void> {
  if (exception) {
    LF_THROW(std::runtime_error("throw requested"));
  }
  co_return;
};

constexpr auto test_void_call = [](auto, bool exception) -> task<void> {
  //
  try_eventually<void> even;

  co_await call(&even, test_void)(exception);

  co_await lf::join;

  // clang-format off

  if(exception){
    REQUIRE(even.has_exception());
  } else {
    REQUIRE(even.empty());
  }

  // clang-format on

  co_return;
};

} // namespace

TEMPLATE_TEST_CASE("Eventually in coro", "[eventually][template]", int, std::vector<int>, int &, int &&) {
  for (bool maybe : {false, bool{LF_COMPILER_EXCEPTIONS}}) {
    lf::sync_wait(lf::unit_pool{}, test_fwd<TestType, false>, maybe);
    lf::sync_wait(lf::unit_pool{}, test_fwd<TestType, true>, maybe);
    lf::sync_wait(lf::unit_pool{}, test_void_call, maybe);
  }
}

// NOLINTEND