// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>

#include "libfork/core/macro.hpp"

// NOLINTBEGIN No linting in tests

consteval void foo() {
  // Test macros valid in constexpr context.
  LF_ASSERT(true);
  LF_LOG("{}", 1);
}

// NOLINTEND