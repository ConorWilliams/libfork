#ifndef AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A
#define AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>         // for bit_cast
#include <exception>   // for rethrow_exception
#include <optional>    // for optional, nullopt
#include <semaphore>   // for binary_semaphore
#include <type_traits> // for conditional_t
#include <utility>     // for forward

#include "libfork/core/eventually.hpp"           // for basic_eventually
#include "libfork/core/ext/handles.hpp"          // for submit_t
#include "libfork/core/ext/list.hpp"             // for intrusive_list
#include "libfork/core/ext/tls.hpp"              // for thread_stack, has_stack
#include "libfork/core/first_arg.hpp"            // for async_function_object
#include "libfork/core/impl/combinate.hpp"       // for quasi_awaitable, y_combinate
#include "libfork/core/impl/frame.hpp"           // for frame
#include "libfork/core/impl/manual_lifetime.hpp" // for manual_lifetime
#include "libfork/core/impl/stack.hpp"           // for stack, swap
#include "libfork/core/invocable.hpp"            // for async_result_t, ignore_t, rootable
#include "libfork/core/macro.hpp"                // for LF_LOG, LF_CLANG_TLS_NOINLINE
#include "libfork/core/scheduler.hpp"            // for scheduler
#include "libfork/core/tag.hpp"                  // for tag, none

/**
 * @file sync_wait.hpp
 *
 * @brief Functionally to enter coroutines from a non-worker thread.
 */

namespace lf {

namespace impl {

// struct tls_stack_swap : immovable<tls_stack_swap> {

//   void make_stack_fresh() {
//     if (stack_is_fresh) {
//       return;
//     }
//     if (!m_this_thread_was_worker) {
//       impl::tls::thread_stack.construct();
//       impl::tls::has_stack = true;
//       return;
//     }
//     m_cache.emplace();                        // Default construct.
//     swap(*m_cache, *impl::tls::thread_stack); // ADL call.
//   }

//   void restore_stack() {
//     if (!stack_is_fresh) {
//       return;
//     }
//     if (!worker) {
//       impl::tls::thread_stack.destroy();
//       impl::tls::has_stack = false;
//     } else {
//       swap(*prev, *impl::tls::thread_stack);
//     }
//   }

//  private:
//   std::optional<impl::stack> m_cache;
//   bool stack_is_fresh = false;
//   bool const m_this_thread_was_worker = impl::tls::has_stack;
// };

} // namespace impl

inline namespace core {

/**
 * @brief Schedule execution of `fun` on `sch` and __block__ until the task is complete.
 *
 * This is the primary entry point from the synchronous to the asynchronous world. A typical libfork program
 * is expected to make a call from `main` into a scheduler/runtime by scheduling a single root-task with this
 * function.
 *
 * This will build a task from `fun` and dispatch it to `sch` via its `schedule` method. Sync wait should
 * __not__ be called by a worker thread (which are never allowed to block) unless the call to `schedule`
 * completes synchronously.
 */
template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
LF_CLANG_TLS_NOINLINE auto sync_wait(Sch &&sch, F fun, Args &&...args) -> async_result_t<F, Args...> {

  std::binary_semaphore sem{0};

  // This is to support a worker sync waiting on work they will launch inline.
  bool worker = impl::tls::has_stack;
  // Will cache workers stack here.
  std::optional<impl::stack> prev = std::nullopt;

  basic_eventually<async_result_t<F, Args...>, true> result;

  impl::y_combinate combinator = combinate<tag::root, modifier::none>(&result, std::move(fun));

  if (!worker) {
    LF_LOG("Sync wait from non-worker thread");
    impl::tls::thread_stack.construct();
    impl::tls::has_stack = true;
  } else {
    LF_LOG("Sync wait from worker thread");
    prev.emplace();                        // Default construct.
    swap(*prev, *impl::tls::thread_stack); // ADL call.
  }

  // This allocates a coroutine on this threads stack.
  impl::quasi_awaitable await = std::move(combinator)(std::forward<Args>(args)...);

  await->set_root_sem(&sem);

  auto *handle = std::bit_cast<impl::submit_t *>(await.release()); // Ownership transferred!

  // TODO: this could be made exception safe.

  [&]() noexcept {
    // If this threw we could clean up coroutine but we would need to repair the worker state.
    impl::ignore_t{} = impl::tls::thread_stack->release();

    if (!worker) {
      impl::tls::thread_stack.destroy();
      impl::tls::has_stack = false;
    } else {
      swap(*prev, *impl::tls::thread_stack);
    }

    typename intrusive_list<impl::submit_t *>::node node{handle};

    // If this threw we could clean up the coroutine if we repaired the worker state.
    std::forward<Sch>(sch).schedule(&node);

    // If this threw we would have to terminate, unless the return variable was stored on the heap.
    sem.acquire();
  }();

  if (result.has_exception()) {
    std::rethrow_exception(std::move(result).exception());
  }

  if constexpr (!std::is_void_v<async_result_t<F, Args...>>) {
    return *std::move(result);
  }
}

} // namespace core

} // namespace lf

#endif /* AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A */
