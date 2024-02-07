#ifndef A090B92E_A266_42C9_BFB0_10681B6BD425
#define A090B92E_A266_42C9_BFB0_10681B6BD425

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception> // for exception

#include "libfork/core/first_arg.hpp" // for quasi_pointer

/**
 * @file exception.hpp
 *
 * @brief Interface for individual exception handling.
 */

namespace lf {

inline namespace core {

/**
 * @brief A concept that checks if a quasi-pointer can be used to stash an exception.
 *
 * If the expression `stash_exception(*ptr)` is well-formed and `noexcept` and `ptr` is
 * used as the return address of an async function, then if that function terminates
 * with an exception, the exception will be stored in the quasi-pointer via a call to
 * `stash_exception`.
 */
template <typename I>
concept stash_exception_in_return = quasi_pointer<I> && requires (I ptr) {
  { stash_exception(*ptr) } noexcept;
};

/**
 * @brief Thrown when a parent knows a child threw an exception but before a join point has been reached.
 *
 * This exception __must__ be caught and then __join must be called__, which will rethrow the child's
 * exception.
 */
struct exception_before_join : std::exception {
  /**
   * @brief A diagnostic message.
   */
  auto what() const noexcept -> char const * override { return "A child threw an exception!"; }
};

} // namespace core

} // namespace lf

#endif /* A090B92E_A266_42C9_BFB0_10681B6BD425 */
