// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp> // for StringRef, TEST_CASE
#include <iostream>                     // for char_traits, basic_ostream, operator<<, basic_os...
#include <thread>                       // for thread

#include "libfork/algorithm/lift.hpp" // for (anonymous), lift
#include "libfork/core.hpp"           // for task, discard_t, sync_wait, y_combinate, join
#include "libfork/schedule.hpp"       // for lazy_pool

using namespace lf;

namespace {

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

inline constexpr auto do_n_jobs = [](auto, int n, int size) -> task<> {
  for (int i = 0; i < n; ++i) {
    co_await lf::fork(lf::lift)(fib, size);
  }
  co_await lf::join;
};

inline constexpr auto cycle = [](auto, int n, int size) -> task<> {
  for (int i = 1; i <= n; ++i) {
    std::cout << "Launching " << i << " jobs\n";
    co_await lf::call(do_n_jobs)(i, size);
  }
};

} // namespace

TEST_CASE("lazy_pool, few jobs", "[observe]") {
  sync_wait(lazy_pool{}, cycle, std::thread::hardware_concurrency(), 40);
}
