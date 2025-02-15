#ifndef C258EF4A_BC44_487A_96BC_6E72746DAAFD
#define C258EF4A_BC44_487A_96BC_6E72746DAAFD

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for same_as
#include <type_traits> // for true_type, false_type

#include "libfork/core/first_arg.hpp" // for referenceable

/**
 * @file safe_ref.hpp
 *
 * @brief A meta-function that verifies that a reference binding is safe.
 */

namespace lf::impl {

// Ban constructs like (T && -> T const &) which would dangle.

namespace detail {

template <typename To, typename From>
struct safe_ref_bind_impl : std::false_type {};

// All reference types can bind to a non-dangling reference of the same kind without dangling.

template <typename T>
struct safe_ref_bind_impl<T, T> : std::true_type {};

// `T const X` can additionally bind to `T X` without dangling//

template <typename To, typename From>
  requires (!std::same_as<To const &, From &>)
struct safe_ref_bind_impl<To const &, From &> : std::true_type {};

template <typename To, typename From>
  requires (!std::same_as<To const &&, From &&>)
struct safe_ref_bind_impl<To const &&, From &&> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that ``To expr = From`` is valid and does not dangle.
 *
 * This requires that ``To`` and ``From`` are both the same reference type or that ``To`` is a
 * const qualified version of ``From``. This explicitly bans conversions like ``T && -> T const &``
 * which would dangle.
 */
template <typename From, typename To>
concept safe_ref_bind_to =                          //
    std::is_reference_v<To> &&                      //
    referenceable<From> &&                          //
    detail::safe_ref_bind_impl<To, From &&>::value; //

} // namespace lf::impl

#endif /* C258EF4A_BC44_487A_96BC_6E72746DAAFD */
