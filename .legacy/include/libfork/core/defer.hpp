#ifndef B4EE570B_F5CF_42CB_9AF3_7376F45FDACC
#define B4EE570B_F5CF_42CB_9AF3_7376F45FDACC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <functional>  // for invoke
#include <type_traits> // for is_nothrow_invocable_v, is_nothrow_constructible_v
#include <utility>     // for forward

#include "libfork/core/impl/utility.hpp" // for immovable
#include "libfork/core/macro.hpp"        // for LF_CONCAT_OUTER, LF_FORCEINLINE

/**
 * @file defer.hpp
 *
 * @brief A Golang-like defer implemented via destructor calls.
 */

namespace lf {

inline namespace core {

/**
 * @brief Basic implementation of a Golang-like defer.
 *
 * \rst
 *
 * Use like:
 *
 * .. code::
 *
 *    auto * ptr = c_api_init();
 *
 *    defer _ = [&ptr] () noexcept {
 *      c_api_clean_up(ptr);
 *    };
 *
 *    // Code that may throw
 *
 * \endrst
 *
 * You can also use the ``LF_DEFER`` macro to create an automatically named defer object.
 *
 */
template <class F>
  requires std::is_nothrow_invocable_v<F>
class [[nodiscard("Defer will execute unless bound to a name!")]] defer : impl::immovable<defer<F>> {
 public:
  /**
   * @brief Construct a new Defer object.
   *
   * @param f Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(F &&f) noexcept(std::is_nothrow_constructible_v<F, F &&>) : m_f(std::forward<F>(f)) {}

  /**
   * @brief Calls the invocable.
   */
  LF_FORCEINLINE constexpr ~defer() noexcept { std::invoke(std::forward<F>(m_f)); }

 private:
  [[no_unique_address]] F m_f;
};

/**
 * @brief A macro to create an automatically named defer object.
 */
#define LF_DEFER ::lf::defer LF_CONCAT_OUTER(at_exit_, __LINE__) = [&]() noexcept

} // namespace core

} // namespace lf

#endif /* B4EE570B_F5CF_42CB_9AF3_7376F45FDACC */
