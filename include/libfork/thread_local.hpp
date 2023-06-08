#ifndef F4C3CE1A_F0F7_485D_8D54_473CCE8294DC
#define F4C3CE1A_F0F7_485D_8D54_473CCE8294DC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <stdexcept>

#include "libfork/macro.hpp"

namespace lf {

/**
 * @brief Store a thread_local static pointer to a T object.
 */
template <typename T>
class thread_local_ptr {
public:
  struct not_set : std::runtime_error {
    not_set() : std::runtime_error("Thread's pointer is not set!") {}
  };

  static auto get() -> T & {
    if (m_ptr == nullptr) {
#if LIBFORK_COMPILER_EXCEPTIONS
      throw not_set{};
#else
      std::abort();
#endif
    }
    return *m_ptr;
  }

  static auto set(T &ctx) noexcept -> void { m_ptr = std::addressof(ctx); }

private:
  static inline thread_local constinit T *m_ptr = nullptr; // NOLINT
};

} // namespace lf

#endif /* F4C3CE1A_F0F7_485D_8D54_473CCE8294DC */
