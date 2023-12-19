#ifndef DE9399DB_593B_4C5C_A9D7_89B9F2FAB920
#define DE9399DB_593B_4C5C_A9D7_89B9F2FAB920

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/core/ext/tls.hpp"

#include "libfork/core/ext/handles.hpp"

/**
 * @file resume.hpp
 *
 * @brief Functions to resume stolen and submitted task.
 */

namespace lf {

inline namespace ext {

/**
 * @brief Resume a task at a submission point.
 */
inline void resume(submit_handle ptr) noexcept {

  LF_LOG("Call to resume on submitted task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  *impl::tls::stack() = impl::stack{frame->stacklet()};

  frame->self().resume();
}

/**
 * @brief Resume a stolen task.
 */
inline void resume(task_handle ptr) noexcept {

  LF_LOG("Call to resume on stolen task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  frame->fetch_add_steal();

  frame->self().resume();
}

} // namespace ext

} // namespace lf

#endif /* DE9399DB_593B_4C5C_A9D7_89B9F2FAB920 */
