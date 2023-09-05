

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/core/exception.hpp"

namespace lf::detail {

/**
 * @brief Stash an exception, thread-safe.
 */
void exception_packet::unhandled_exception() noexcept {
#if LF_PROPAGATE_EXCEPTIONS
  if (!m_ready.test_and_set(std::memory_order_acq_rel)) {
    LF_LOG("Exception saved");
    m_exception = std::current_exception();
    LF_ASSERT(m_exception);
  } else {
    LF_LOG("Exception discarded");
  }
#else
  noexcept_invoke([] { LF_RETHROW; });
#endif
}

/**
 * @brief Rethrow if any children threw an exception.
 *
 * This should be called only when a thread is at a join point.
 */
void exception_packet::rethrow_if_unhandled() {
#if LF_PROPAGATE_EXCEPTIONS
  if (m_exception) {
    LF_LOG("Rethrowing exception");
    // We are the only thread that can touch this until a steal, which provides the required syncronisation.
    m_ready.clear(std::memory_order_relaxed);
    std::rethrow_exception(std::exchange(m_exception, {}));
  }
#endif
}

} // namespace lf::detail
