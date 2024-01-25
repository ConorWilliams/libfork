// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// #define NDEBUG
// #define LF_COROUTINE_OFFSET 2 * sizeof(void *)

#include <functional>
#include <type_traits>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "matrix.hpp"

#include "libfork/algorithm/fold.hpp"

#include "libfork/core.hpp"
#include "libfork/schedule.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

namespace {

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

template <typename F>
auto doubler(F fun) {
  return [fun]<class T>(auto /* unused */, T x) -> task<T> {
    co_return 2 * co_await lf::just(fun)(std::forward<T>(x));
  };
}

template <typename Sch, typename F, typename Proj = std::identity>
void test(Sch &&sch, F sum, Proj proj = {}) {
  //

  std::span<int> oops;

  // Empty cases
  REQUIRE(lf::sync_wait(sch, lf::fold, oops, sum, proj) == std::nullopt);
  REQUIRE(lf::sync_wait(sch, lf::fold, oops.begin(), oops.end(), sum, proj) == std::nullopt);
  REQUIRE(lf::sync_wait(sch, lf::fold, oops, 10, sum, proj) == std::nullopt);
  REQUIRE(lf::sync_wait(sch, lf::fold, oops.begin(), oops.end(), 10, sum, proj) == std::nullopt);

  std::vector<int> v;

  constexpr int n = 10'000;

  for (auto i = 1; i <= n; i++) {
    v.push_back(i);
  }

  constexpr int correct = n * (n + 1) / 2;

  //   Check grain = 1 case:
  REQUIRE(lf::sync_wait(sch, lf::fold, v, sum, proj) == correct);
  REQUIRE(lf::sync_wait(sch, lf::fold, v.begin(), v.end(), sum, proj) == correct);
  REQUIRE(lf::sync_wait(sch, lf::fold, v, sum, doubler(proj)) == 2 * correct);
  REQUIRE(lf::sync_wait(sch, lf::fold, v.begin(), v.end(), sum, doubler(proj)) == 2 * correct);

  for (auto m : {100, 300, 20'000}) {
    REQUIRE(lf::sync_wait(sch, lf::fold, v, m, sum, proj) == correct);
    REQUIRE(lf::sync_wait(sch, lf::fold, v.begin(), v.end(), m, sum, proj) == correct);
    REQUIRE(lf::sync_wait(sch, lf::fold, v, m, sum, doubler(proj)) == 2 * correct);
    REQUIRE(lf::sync_wait(sch, lf::fold, v.begin(), v.end(), m, sum, doubler(proj)) == 2 * correct);
  }

#ifndef _MSC_VER

  // ----------- Now with small inputs ----------- //

  REQUIRE(lf::sync_wait(sch, fold, std::span(v.data(), 4), sum, proj) == 4 * (4 + 1) / 2);
  REQUIRE(lf::sync_wait(sch, fold, std::span(v.data(), 3), sum, proj) == 3 * (3 + 1) / 2);
  REQUIRE(lf::sync_wait(sch, fold, std::span(v.data(), 2), sum, proj) == 2 * (2 + 1) / 2);
  REQUIRE(lf::sync_wait(sch, fold, std::span(v.data(), 1), sum, proj) == 1 * (1 + 1) / 2);

#endif
}

constexpr auto sum_reg = std::plus<>{};

constexpr auto sum_coro = [](auto, auto const a, auto const b) -> task<decltype(a + b)> {
  co_return a + b;
};

constexpr auto coro_identity = []<typename T>(auto, T &&val) -> task<T &&> {
  co_return std::forward<T>(val);
};

} // namespace

TEMPLATE_TEST_CASE("fold (reg, reg)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), sum_reg);
}

TEMPLATE_TEST_CASE("fold (co, reg)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), sum_coro);
}

TEMPLATE_TEST_CASE("fold (reg, co)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), sum_reg, coro_identity);
}

TEMPLATE_TEST_CASE("fold (co, co)", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {
  test(make_scheduler<TestType>(), sum_coro, coro_identity);
}

TEMPLATE_TEST_CASE("fold non-commuting", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  constexpr std::size_t max_elems = 50;

  for (std::size_t n = 1; n < max_elems; n++) {
    for (int chunk = 1; chunk <= static_cast<int>(max_elems) + 1; chunk++) {

      std::vector<matrix> const in = random_vec(std::type_identity<matrix>{}, n);

      matrix ngv = std::reduce(in.begin(), in.end(), matrix{1, 0, 0, 1}, std::multiplies<>{});

      REQUIRE(ngv == lf::sync_wait(sch, lf::fold, in, chunk, std::multiplies<>{}));
    }
  }
}
