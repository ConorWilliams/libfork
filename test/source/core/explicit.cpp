// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <random>
#include <stdexcept>
#include <thread>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"
#include "libfork/schedule.hpp"

using namespace lf;

namespace {

inline constexpr auto r_fib = [](auto fib, int n) -> lf::task<int> {
  //

  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(&a, fib)(n - 1);
  co_await lf::call(&b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

inline constexpr auto sch_on = [](auto sch_on, context *target) -> task<bool> {
  //
  co_await target;

  if (sch_on.context() != target) {
    co_return false;
  }

  int n;

  co_await lf::call(&n, r_fib)(6);

  co_return n == 8;
};

inline constexpr auto loop = [](auto loop, int n, std::vector<context *> neigh) -> task<bool> {
  //

  std::span res = co_await co_new<int>(n == 0 ? neigh.size() : n);

  if (n == 0) {
    for (int i = 0; i < res.size(); ++i) {
      if (n % 2 == 0) {
        co_await lf::fork(&res[i], sch_on)(neigh[i]);
      } else {
        co_await lf::call(&res[i], sch_on)(neigh[i]);
      }
    }
  } else {

    lf::xoshiro rng{seed, std::random_device{}};

    for (int i = 0; i < res.size(); ++i) {

      std::ranges::shuffle(neigh, rng);

      if (n % 2 == 0) {
        co_await lf::fork(&res[i], loop)(n - 1, neigh);
      } else {
        co_await lf::call(&res[i], loop)(n - 1, neigh);
      }
    }
  }

  co_await lf::join;

  bool ok = std::ranges::all_of(res, std::identity{});

  co_await co_delete(res);

  co_return ok;
};

} // namespace

//  unit_pool, debug_pool, busy_pool, lazy_pool

TEMPLATE_TEST_CASE("Explicit scheduling", "[explicit][template]", busy_pool, lazy_pool) {

  TestType sch{std::min(4U, std::thread::hardware_concurrency())};

  std::vector contexts(sch.contexts().begin(), sch.contexts().end());

  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < 5; ++j) {
      REQUIRE(sync_wait(sch, loop, j, contexts));
    }
  }
}

namespace {

inline constexpr auto sfib = [](auto sfib, int n, std::span<context *> neigh, lf::xoshiro rng) -> task<int> {
  //
  if (n < 2) {
    co_return n;
  }

  if (rng() % 2 == 0) {

    std::uniform_int_distribution<std::size_t> dist{0, neigh.size() - 1};

    context *target = neigh[dist(rng)];

    co_await target;

    if (sfib.context() != target) {
      co_return -1;
    }
  }

  int a, b;

  co_await lf::fork(&a, sfib)(n - 1, neigh, lf::xoshiro{seed, rng});
  co_await lf::call(&b, sfib)(n - 2, neigh, lf::xoshiro{seed, rng});

  co_await lf::join;

  if (a == -1 || b == -1) {
    co_return -1;
  }

  co_return a + b;
};

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

} // namespace

TEMPLATE_TEST_CASE("Explicit fibonacci", "[explicit][template]", busy_pool, lazy_pool) {

  TestType sch{std::min(4U, std::thread::hardware_concurrency())};

  for (int i = 0; i < 100; ++i) {
    for (int j = 1; j < 20; ++j) {
      REQUIRE(sync_wait(sch, sfib, j, sch.contexts(), lf::xoshiro{seed, std::random_device{}}) == fib(j));
    }
  }
}