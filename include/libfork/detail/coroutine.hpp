#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <__coroutine/coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <version>

/**
 * @file coroutine.hpp
 *
 * @brief Use a version of coroutines if available.
 */

#if __has_include(<coroutine>)  // Check for a standard library

  #include <coroutine>

namespace lf {

using std::coroutine_handle;  // NOLINT
using std::noop_coroutine;    // NOLINT
using std::suspend_always;    // NOLINT
using std::suspend_never;     // NOLINT

}  // namespace lf

#elif __has_include(<experimental/optional>)  // Check for an experimental version

  #include <experimental/optional>

namespace lf {

using std::experimental::coroutine_handle;  // NOLINT
using std::experimental::noop_coroutine;    // NOLINT
using std::experimental::suspend_always;    // NOLINT
using std::experimental::suspend_never;     // NOLINT

}  // namespace lf
#else
  #error "Missing <coroutine> header!"
#endif
