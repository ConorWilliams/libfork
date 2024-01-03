// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>
#include <libfork/core/macro.hpp>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

namespace {

using lf::call;
using lf::fork;
using lf::join;

using lf::task;

constexpr auto fib = [](auto fib, int n) -> task<int> {
  //
  if (n < 2) {
    co_return n;
  }

  int a, b;

  std::exception_ptr __exception_ptr;

  try {
    co_await fork[a, fib](n - 1);
    co_await call[b, fib](n - 2);
  } catch (...) {
    __exception_ptr = std::current_exception();
    goto __exception;
  }

  co_await join;
  goto __fallthrough;

__exception:
  LF_ASSERT(__exception_ptr);
  co_await join;
  std::rethrow_exception(std::move(__exception_ptr));

__fallthrough:

  co_return a + b;
};

} // namespace

// NOLINTEND