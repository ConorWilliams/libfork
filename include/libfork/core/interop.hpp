#ifndef BDE6CBCC_7576_4082_AAC5_2A207FEA9293
#define BDE6CBCC_7576_4082_AAC5_2A207FEA9293

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>
#include <utility>

#include "libfork/core/ext/handles.hpp" // for  submit_handle
#include "libfork/core/ext/list.hpp"
#include "libfork/core/ext/tls.hpp"

/**
 * @file interop.hpp
 *
 * @brief Machinery for interfacing with external workers.
 */

namespace lf {

inline namespace core {

/**
 * @brief Defines the interface for external awaitables.
 *
 * An external awaitable can be awaited inside a libfork coroutine. If the external awaitable
 * is not ready then the coroutine will be suspended and a submit_handle will be passed to the
 * external awaitor's ``await_suspend()`` function. This can then be resumed by any worker as
 * normal.
 */
template <typename T>
concept external_awaitable = //
    std::constructible_from<std::remove_cvref_t<T>, T &&> &&
    requires (std::remove_cvref_t<T> awaiter, intruded_list<submit_handle> handle) {
      { awaiter.await_ready() } -> std::convertible_to<bool>;
      { awaiter.await_suspend(handle) } -> std::same_as<void>;
      { awaiter.await_resume() };
    };

/**
 * @brief An external awaitable to explicitly transfer execution to another worker.
 */
struct [[nodiscard]] schedule_on_context {
 private:
  worker_context *m_dest;

  explicit schedule_on_context(worker_context *dest) noexcept : m_dest{dest} {}

  friend auto resume_on(worker_context *dest) noexcept -> schedule_on_context;

 public:
  /**
   * @brief Don't suspend if on correct context.
   */
  auto await_ready() const noexcept { return impl::tls::context() == m_dest; }

  /**
   * @brief Reschedule this coroutine onto the requested destination.
   */
  auto await_suspend(intruded_list<submit_handle> handle) noexcept -> void { m_dest->submit(handle); }

  /**
   * @brief A no-op.
   */
  auto await_resume() const noexcept -> void {}
};

static_assert(external_awaitable<schedule_on_context>);

/**
 * @brief Create an external awaitable to explicitly transfer execution to ``dest``.
 */
inline auto resume_on(worker_context *dest) noexcept -> schedule_on_context {
  return schedule_on_context{non_null(dest)};
}

} // namespace core

} // namespace lf

#endif /* BDE6CBCC_7576_4082_AAC5_2A207FEA9293 */
