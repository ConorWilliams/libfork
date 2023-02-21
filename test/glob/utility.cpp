// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>

#include <catch2/catch_test_macros.hpp>

// !BEGIN-EXAMPLE

#include "libfork/utility.hpp"

void add_count(std::vector<int>& counts, int val) {
  //
  bool commit = false;

  counts.push_back(val);  //  (1) direct action.

  lf::defer _ = [&]() noexcept {
    if (!commit) {
      counts.pop_back();  // (2) rollback action.
    }
  };

  //                         (3) other operations that may throw.

  commit = true;  //         (4) disable rollback actions if no throw.

  // Lambda executed when scope exits (function returns or exception).
}

// !END-EXAMPLE

// NOLINTBEGIN No linting in tests

consteval void foo() {
  ASSERT(true, "test macro valid in constexpr context.");
  DEBUG_TRACKER("test macro valid in constexpr context.");
}

// NOLINTEND