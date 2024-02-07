#ifndef BDE6CBCC_7576_4082_AAC5_2A207FEA9293
#define BDE6CBCC_7576_4082_AAC5_2A207FEA9293

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for convertible_to, same_as
#include <type_traits> // for remove_cvref_t
#include <utility>     // for declval, forward

#include "libfork/core/ext/context.hpp"  // for worker_context
#include "libfork/core/ext/handles.hpp"  // for submit_handle
#include "libfork/core/ext/tls.hpp"      // for context
#include "libfork/core/first_arg.hpp"    // for storable
#include "libfork/core/impl/utility.hpp" // for non_null

/**
 * @file scheduler.hpp
 *
 * @brief Machinery for interfacing with scheduling coroutines.
 */

namespace lf {

inline namespace core {

/**
 * @brief A concept that schedulers must satisfy.
 *
 * This requires only a single method, `schedule` which accepts an `lf::submit_handle` and
 * promises to call `lf::resume()` on it. The `schedule method should fulfill the strong
 * exception guarantee.
 */
template <typename Sch>
concept scheduler = requires (Sch &&sch, submit_handle handle) {
  std::forward<Sch>(sch).schedule(handle); //
};

/**
 * @brief Defines the interface for awaitables that may trigger a context switch.
 *
 * A ``context_switcher`` can be awaited inside a libfork coroutine. If the awaitable
 * is not ready then the coroutine will be suspended and a submit_handle will be passed to the
 * context switcher's ``await_suspend()`` function. This can then be resumed by any worker as
 * normal.
 */
template <typename T>
concept context_switcher = storable<T> && requires (std::remove_cvref_t<T> awaiter, submit_handle handle) {
  { awaiter.await_ready() } -> std::convertible_to<bool>;
  { awaiter.await_suspend(handle) } -> std::same_as<void>;
  { awaiter.await_resume() };
};

template <scheduler Sch>
struct resume_on_quasi_awaitable;

/**
 * @brief Create an ``lf::core::context_switcher`` to explicitly transfer execution to ``dest``.
 *
 * `dest` must be non-null.
 */
template <scheduler Sch>
auto resume_on(Sch *dest) noexcept -> resume_on_quasi_awaitable<Sch> {
  return resume_on_quasi_awaitable<Sch>{non_null(dest)};
}

/**
 * @brief An ``lf::core::context_switcher`` that just transfers execution to a new scheduler.
 */
template <scheduler Sch>
struct [[nodiscard("This should be immediately co_awaited")]] resume_on_quasi_awaitable {
 private:
  Sch *m_dest;

  explicit resume_on_quasi_awaitable(worker_context *dest) noexcept : m_dest{dest} {}

  friend auto resume_on<Sch>(Sch *dest) noexcept -> resume_on_quasi_awaitable;

 public:
  /**
   * @brief Don't suspend if on correct context.
   */
  auto await_ready() const noexcept { return impl::tls::context() == m_dest; }

  /**
   * @brief Reschedule this coroutine onto the requested destination.
   */
  auto
  await_suspend(submit_handle handle) noexcept(noexcept(std::declval<Sch *>()->schedule(handle))) -> void {
    m_dest->schedule(handle);
  }

  /**
   * @brief A no-op.
   */
  static auto await_resume() noexcept -> void {}
};

static_assert(context_switcher<resume_on_quasi_awaitable<worker_context>>);

} // namespace core

} // namespace lf

#endif /* BDE6CBCC_7576_4082_AAC5_2A207FEA9293 */
