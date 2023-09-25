#ifndef C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC
#define C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "libfork/core.hpp"

#include "libfork/schedule/contexts.hpp"

/**
 * @file unit_pool.hpp
 *
 * @brief A scheduler that runs all tasks inline on the current thread.
 */

namespace lf {

namespace impl {

template <thread_context Context>
class unit_pool_impl : impl::immovable<unit_pool_impl<Context>> {
 public:
  using context_type = Context;

  static void schedule(intruded_h<context_type> *ptr) { context_type::submit(ptr); }

  unit_pool_impl() { worker_init(&m_context); }

  ~unit_pool_impl() { worker_finalize(&m_context); }

 private:
  [[no_unique_address]] context_type m_context;
};

} // namespace impl

inline namespace ext {

/**
 * @brief A scheduler that runs all tasks inline on the current thread and keeps an internal stack.
 *
 * This is exposed/intended for testing, using this thread pool is equivalent to
 * using a `busy_pool` with a single thread. It is different from `unit_pool` in that
 * it explicitly disables the `fork` -> `call` optimisation.
 */
using debug_pool = impl::unit_pool_impl<impl::test_immediate_context>;

static_assert(scheduler<debug_pool>);

} // namespace ext

/**
 * @brief A scheduler that runs all tasks inline on the current thread.
 *
 * This is useful for testing/debugging/benchmarking.
 */
using unit_pool = impl::unit_pool_impl<impl::immediate_context>;

static_assert(scheduler<unit_pool>);

} // namespace lf

#endif /* C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC */
