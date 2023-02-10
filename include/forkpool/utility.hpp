#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>
#include <iostream>
#include <string_view>

/**
 * @file utility.hpp
 *
 * @brief A small collection of utilities.
 *
 */

namespace fp {

struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

#ifndef NDEBUG

namespace detail {

constexpr auto file_name(std::string_view path) -> std::string_view {
  if (auto last = path.find_last_of("/\\"); last != std::string_view::npos) {
    path.remove_prefix(last);
  }
  return path;
}

}  // namespace detail

/**
 * @brief Use like fly::verify() but disabled if ``NDEBUG`` defined.
 */
#define ASSERT(expr, msg)                                                                       \
  do {                                                                                          \
    constexpr std::string_view fname = ::fp::detail::file_name(__FILE__);                       \
                                                                                                \
    if (!(expr)) {                                                                              \
      std::cerr << "ASSERT \"" << #expr << "\" failed in " << fname << ":" << __LINE__ << " | " \
                << (msg) << std::endl;                                                          \
      std::terminate();                                                                         \
    }                                                                                           \
  } while (false)

#else

/**
 * @brief Use like fly::verify() but disabled if ``NDEBUG`` defined.
 */
#define ASSERT(...) \
  do {              \
  } while (false)

#endif  // !NDEBUG

}  // namespace fp