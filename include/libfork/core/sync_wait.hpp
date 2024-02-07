#ifndef AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A
#define AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>         // for bit_cast
#include <exception>   // for rethrow_exception
#include <memory>      // for make_shared
#include <optional>    // for optional
#include <semaphore>   // for binary_semaphore
#include <type_traits> // for conditional_t
#include <utility>     // for forward

#include "libfork/core/defer.hpp"                // for LF_DEFER
#include "libfork/core/eventually.hpp"           // for try_eventually
#include "libfork/core/exception.hpp"            // for sync_wait_in_worker
#include "libfork/core/ext/handles.hpp"          // for submit_node_t, submit_t
#include "libfork/core/ext/list.hpp"             // for intrusive_list
#include "libfork/core/ext/tls.hpp"              // for has_stack, thread_stack, has_context
#include "libfork/core/first_arg.hpp"            // for async_function_object
#include "libfork/core/impl/combinate.hpp"       // for quasi_awaitable, y_combinate
#include "libfork/core/impl/frame.hpp"           // for frame
#include "libfork/core/impl/manual_lifetime.hpp" // for manual_lifetime
#include "libfork/core/impl/stack.hpp"           // for stack
#include "libfork/core/invocable.hpp"            // for async_result_t, ignore_t, rootable
#include "libfork/core/macro.hpp"                // for LF_CLANG_TLS_NOINLINE
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
 * This will build a task from `fun` and dispatch it to `sch` via its `schedule` method. If `sync_wait` is
 * called by a worker thread (which are never allowed to block) then `lf::core::sync_wait_in_worker` will be
 * thrown.
 */
template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
LF_CLANG_TLS_NOINLINE auto sync_wait(Sch &&sch, F fun, Args &&...args) -> async_result_t<F, Args...> {

  std::binary_semaphore sem{0};

  if (impl::tls::has_stack || impl::tls::has_context) {
    throw sync_wait_in_worker{};
  }

  // Initialize the non-workers stack.
  impl::tls::thread_stack.construct();
  impl::tls::has_stack = true;

  // Clean up the stack on exit.
  LF_DEFER {
    impl::tls::thread_stack.destroy();
    impl::tls::has_stack = false;
  };

  using eventually_t = try_eventually<async_result_t<F, Args...>>;

  // If we fail to wait for the result to complete due to an exception we will need
  // to detach the coroutine, this will require the node and the result to be stored
  // on the heap such that returning from this function will not destroy them.
  struct heap_alloc : eventually_t {

    using eventually_t::operator=;

    auto operator=(heap_alloc &&other) -> heap_alloc & = delete;
    auto operator=(heap_alloc const &other) -> heap_alloc & = delete;

    std::optional<impl::submit_node_t> node; // Make default constructible.
  };

  auto heap = std::make_shared<heap_alloc>();

  // Build a combinator, copies heap shared_ptr.
  impl::y_combinate combinator = combinate<tag::root, modifier::none>(heap, std::move(fun));
  // This allocates a coroutine on this threads stack.
  impl::quasi_awaitable await = std::move(combinator)(std::forward<Args>(args)...);
  // Set the root semaphore.
  await->set_root_sem(&sem);

  // If this throws then `await` will clean up the coroutine.
  impl::ignore_t{} = impl::tls::thread_stack->release();

  // We will pass a pointer to this to schedule.
  heap->node.emplace(std::bit_cast<impl::submit_t *>(await.get()));

  // Schedule upholds the strong exception guarantee hence, if it throws `await` cleans up.
  std::forward<Sch>(sch).schedule(&*heap->node);
  // If -^ didn't throw then we release ownership of the coroutine.
  impl::ignore_t{} = await.release();

  // If this throws that's ok as `result` and `node` are on the heap.
  sem.acquire();

  if (heap->has_exception()) {
    std::rethrow_exception(std::move(*heap).exception());
  }

  if constexpr (!std::is_void_v<async_result_t<F, Args...>>) {
    return *std::move(*heap);
  }
}

} // namespace core

} // namespace lf

#endif /* AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A */
