// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// NOLINTBEGIN No linting in tests

#include "libfork/core.hpp" // for LF_ASSERT, LF_LOG

consteval void foo() {
  // Test macros valid in constexpr context.
  LF_ASSERT(true);
  LF_LOG("{}", 1);
}

// NOLINTEND