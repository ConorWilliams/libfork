#ifndef A090B92E_A266_42C9_BFB0_10681B6BD425
#define A090B92E_A266_42C9_BFB0_10681B6BD425

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/core/first_arg.hpp" // for quasi_pointer

/**
 * @file exception.hpp
 *
 * @brief Interface for individual exception handling.
 */

namespace lf {

inline namespace core {

/**
 * @brief A concept that requires a type can store an exception.
 */
template <typename I>
concept stash_exception_in_return = quasi_pointer<I> && requires (I ptr) {
  { stash_exception(*ptr) } noexcept;
};

} // namespace core

} // namespace lf

#endif /* A090B92E_A266_42C9_BFB0_10681B6BD425 */
