#ifndef A6BE090F_9077_40E8_9B57_9BAFD9620469
#define A6BE090F_9077_40E8_9B57_9BAFD9620469

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "libfork/core/co_alloc.hpp"
#include "libfork/core/control_flow.hpp"
#include "libfork/core/defer.hpp"
#include "libfork/core/eventually.hpp"
#include "libfork/core/exception.hpp"
#include "libfork/core/first_arg.hpp"
#include "libfork/core/invocable.hpp"
#include "libfork/core/just.hpp"
#include "libfork/core/macro.hpp"
#include "libfork/core/scheduler.hpp"
#include "libfork/core/sync_wait.hpp"
#include "libfork/core/tag.hpp"
#include "libfork/core/task.hpp"

#include "libfork/core/ext/context.hpp"
#include "libfork/core/ext/deque.hpp"
#include "libfork/core/ext/handles.hpp"
#include "libfork/core/ext/list.hpp"
#include "libfork/core/ext/resume.hpp"
#include "libfork/core/ext/tls.hpp"

#include "libfork/core/impl/awaitables.hpp"
#include "libfork/core/impl/combinate.hpp"
#include "libfork/core/impl/frame.hpp"
#include "libfork/core/impl/manual_lifetime.hpp"
#include "libfork/core/impl/promise.hpp"
#include "libfork/core/impl/return.hpp"
#include "libfork/core/impl/stack.hpp"
#include "libfork/core/impl/utility.hpp"

/**
 * @file core.hpp
 *
 * @brief Meta header which includes all the headers in ``libfork/core``.
 *
 * This header is a single include which provides the minimal set of headers required
 * for using `libfork` s "core" API. If you are writing your own schedulers and not using any
 * of `libfork` s "extension" API then this is all you need.
 */

#endif /* A6BE090F_9077_40E8_9B57_9BAFD9620469 */
