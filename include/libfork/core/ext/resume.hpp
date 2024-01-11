#ifndef DE9399DB_593B_4C5C_A9D7_89B9F2FAB920
#define DE9399DB_593B_4C5C_A9D7_89B9F2FAB920

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>       // for bit_cast
#include <coroutine> // for coroutine_handle

#include "libfork/core/ext/context.hpp" // for full_context
#include "libfork/core/ext/handles.hpp" // for submit_handle, task_handle
#include "libfork/core/ext/tls.hpp"     // for context, stack
#include "libfork/core/impl/frame.hpp"  // for frame
#include "libfork/core/impl/stack.hpp"  // for stack
#include "libfork/core/macro.hpp"       // for LF_ASSERT_NO_ASSUME, LF_LOG

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

  if (frame->load_steals() == 0) {
    impl::stack *stack = impl::tls::stack();
    LF_ASSERT(stack->empty());
    *stack = impl::stack{frame->stacklet()};
  } else {
    LF_ASSERT_NO_ASSUME(impl::tls::stack()->empty());
  }

  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
  frame->self().resume();
  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
  LF_ASSERT_NO_ASSUME(impl::tls::stack()->empty());
}

/**
 * @brief Resume a stolen task.
 */
inline void resume(task_handle ptr) noexcept {

  LF_LOG("Call to resume on stolen task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  frame->fetch_add_steal();

  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
  LF_ASSERT_NO_ASSUME(impl::tls::stack()->empty());
  frame->self().resume();
  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
  LF_ASSERT_NO_ASSUME(impl::tls::stack()->empty());
}

} // namespace ext

} // namespace lf

#endif /* DE9399DB_593B_4C5C_A9D7_89B9F2FAB920 */
