#ifndef FE9C96B0_5DDD_4438_A3B0_E77BD54F8673
#define FE9C96B0_5DDD_4438_A3B0_E77BD54F8673

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <version>

/**
 * @file coroutine.hpp
 *
 * @brief Includes \<coroutine\> or \<experimental/coroutine\> depending on the compiler.
 */

// NOLINTBEGIN

#ifdef __has_include
  #if __has_include(<coroutine>) // Check for a standard library
    #include <coroutine>
namespace lf {
namespace stdx = std;
}
  #elif __has_include(<experimental/coroutine>) // Check for an experimental version.
    #include <experimental/coroutine>
namespace lf {
namespace stdx = std::experimental;
}
  #else
    #error "Missing <coroutine> header!"
  #endif
#else
  #include <coroutine>
namespace lf {
namespace stdx = std;
}
#endif

// NOLINTEND

#endif /* FE9C96B0_5DDD_4438_A3B0_E77BD54F8673 */
