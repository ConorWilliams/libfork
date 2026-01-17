// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for INTERNAL_CATCH_NOINTERNAL_CATCH_DEF

#include <cstddef>
#include <ranges>

#include "libfork/algorithm/for_each.hpp" // for for_each
#include "libfork/core.hpp"               // for sync_wait, task
#include "libfork/core/ext/context.hpp"
#include "libfork/schedule.hpp" // for busy_pool, lazy_pool, unit_pool

// NOLINTBEGIN No linting in tests

using namespace lf;

namespace {

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{1};
  } else {
    return T{};
  }
}

} // namespace

struct pair {
  worker_context *ctx;
  int *v;
  int n;
};

auto set_on = [](auto /* unused */, pair const &p) -> task<> {
  // Move to the context
  co_await resume_on(p.ctx);
  // Set the valued
  *p.v = p.n;
};

TEMPLATE_TEST_CASE("Explicit + algorithm", "[explicit][algorithm][template]", busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  int N = 10'000;

  for (int n = 0; n < 25; n++) {

    std::vector<int> v(impl::checked_cast<std::size_t>(N));

    std::span contexts = sch.contexts();

    std::vector<pair> pairs;

    for (std::size_t i = 0; i < v.size(); ++i) {
      pairs.push_back({contexts[i % contexts.size()], &v[i], impl::checked_cast<int>(i)});
    }

    sync_wait(sch, for_each, pairs, set_on);

    for (std::size_t i = 0; i < v.size(); ++i) {
      REQUIRE(v[i] == impl::checked_cast<int>(i));
    }
  }
}