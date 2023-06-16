#ifndef B88D7FC0_BD59_4276_837B_3CAAAB9FC2EF
#define B88D7FC0_BD59_4276_837B_3CAAAB9FC2EF

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <exception>
#include <utility>

#include "libfork/macro.hpp"

/**
 * @file exception.hpp
 *
 * @brief A small type that encapsulates exceptions thrown by children.
 */

namespace lf::detail {

struct immovable {
  immovable() = default;
  immovable(const immovable &) = delete;
  immovable(immovable &&) = delete;
  auto operator=(const immovable &) -> immovable & = delete;
  auto operator=(immovable &&) -> immovable & = delete;
  ~immovable() = default;
};

static_assert(std::is_empty_v<immovable>);

#if LIBFORK_PROPAGATE_EXCEPTIONS

/**
 * @brief A thread safe std::exception_ptr.
 */
class exception_packet : immovable {
public:
  /**
   * @brief Test if currently storing an exception.
   */
  explicit operator bool() const noexcept { return static_cast<bool>(m_exception); }

  /**
   * @brief Stash an exception, thread-safe.
   */
  void unhandled_exception() noexcept {
    if (!m_ready.test_and_set(std::memory_order_acq_rel)) {
      LIBFORK_LOG("Exception saved");
      m_exception = std::current_exception();
      LIBFORK_ASSERT(m_exception);
    } else {
      LIBFORK_LOG("Exception discarded");
    }
  }

  /**
   * @brief Rethrow if any children threw an exception.
   *
   * This should be called only when a thread is at a join point.
   */
  void rethrow_if_unhandled() {
    if (m_exception) {
      LIBFORK_LOG("Rethrowing exception");
      // We are the only thread that can touch this until a steal, which provides the required syncronisation.
      m_ready.clear(std::memory_order_relaxed);
      std::rethrow_exception(std::exchange(m_exception, {}));
    }
  }

private:
  std::exception_ptr m_exception = nullptr;    ///< Exceptions thrown by children.
  std::atomic_flag m_ready = ATOMIC_FLAG_INIT; ///< Whether the exception is ready.
};

#else

class exception_packet {
public:
  explicit constexpr operator bool() const noexcept { return false; }

  void unhandled_exception() noexcept {
  #if LIBFORK_COMPILER_EXCEPTIONS
    throw;
  #else
    std::abort();
  #endif
  }

  void rethrow_if_unhandled() noexcept {}
};

#endif

} // namespace lf::detail

#endif /* B88D7FC0_BD59_4276_837B_3CAAAB9FC2EF */
