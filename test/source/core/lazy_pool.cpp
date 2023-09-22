/*
 * Copyright (c) Conor Williams, Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libfork/algorithm/lift.hpp"
#include "libfork/core.hpp"
#include "libfork/schedule/busy_pool.hpp"
#include "libfork/schedule/lazy_pool.hpp"

using namespace lf;

namespace {

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

inline constexpr async do_n_jobs = [](auto, int n, int size) -> task<> {
  for (int i = 0; i < n; ++i) {
    co_await lf::fork(LF_LIFT2(fib))(size);
  }
  co_await lf::join;
};

inline constexpr async cycle = [](auto, int n, int size) -> task<> {
  for (int i = 1; i <= n; ++i) {
    std::cout << "Launching " << i << " jobs\n";
    co_await do_n_jobs(i, size);
  }
};

} // namespace

TEST_CASE("lazy_pool, few jobs", "[observe]") { sync_wait(lazy_pool{}, cycle, std::thread::hardware_concurrency(), 40); }
