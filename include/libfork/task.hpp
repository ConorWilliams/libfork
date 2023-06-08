#ifndef C9921D3E_28E4_4577_BB9C_E7CA55766E92
#define C9921D3E_28E4_4577_BB9C_E7CA55766E92

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include "libfork/coroutine.hpp"
#include "libfork/macro.hpp"
#include "libfork/promise.hpp"
#include "libfork/promise_base.hpp"

/**
 * @file task.hpp
 *
 * @brief The ``lf::task`` class.
 */

namespace lf {

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T, thread_context Context>
  requires(!std::is_reference_v<T>)
class task : public stdexp::coroutine_handle<detail::promise_base<T>> {
public:
  using value_type = T;         ///< The type of the value returned by the coroutine.
  using context_type = Context; ///< The type of the context in which the coroutine is executed.

private:
  explicit constexpr task(auto handle) noexcept : stdexp::coroutine_handle<detail::promise_base<T>>{handle} {}

  template <typename, thread_context, detail::tag>
  friend struct detail::promise_type;
};

} // namespace lf

#endif /* C9921D3E_28E4_4577_BB9C_E7CA55766E92 */
