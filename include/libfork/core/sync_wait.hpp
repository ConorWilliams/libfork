#ifndef AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A
#define AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>         // for bit_cast
#include <exception>   // for exception_ptr, reth...
#include <optional>    // for optional, nullopt
#include <type_traits> // for conditional_t
#include <utility>     // for forward, move

#include "libfork/core/eventually.hpp"           // for eventually
#include "libfork/core/ext/handles.hpp"          // for submit_handle
#include "libfork/core/ext/list.hpp"             // for intruded_list, intr...
#include "libfork/core/ext/tls.hpp"              // for thread_stack, has_s...
#include "libfork/core/first_arg.hpp"            // for async_function_object
#include "libfork/core/impl/combinate.hpp"       // for quasi_awaitable
#include "libfork/core/impl/frame.hpp"           // for root_notify, frame
#include "libfork/core/impl/manual_lifetime.hpp" // for manual_lifetime
#include "libfork/core/impl/stack.hpp"           // for stack, swap
#include "libfork/core/impl/utility.hpp"         // for empty
#include "libfork/core/invocable.hpp"            // for async_result_t, ign...
#include "libfork/core/macro.hpp"                // for LF_LOG, LF_CLANG_TL...
#include "libfork/core/tag.hpp"                  // for tag

/**
 * @file sync_wait.hpp
 *
 * @brief Functionally to enter coroutines from a non-worker thread.
 */

namespace lf {

namespace impl {

struct empty;

} // namespace impl

inline namespace core {

/**
 * @brief A concept that schedulers must satisfy.
 *
 * This requires only a single method, `schedule` which accepts an `lf::intruded_list<submit_handle>` and
 * promises to call `lf::resume()` on it.
 */
template <typename Sch>
concept scheduler = requires (Sch &&sch, intruded_list<submit_handle> handle) {
  std::forward<Sch>(sch).schedule(handle); //
};

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

  using R = async_result_t<F, Args...>;
  constexpr bool is_void = std::is_void_v<R>;

  impl::root_notify notifier;
  eventually<std::conditional_t<is_void, impl::empty, R>> result;

  // This is to support a worker sync waiting on work they will launch inline.
  bool worker = impl::tls::has_stack;
  // Will cache workers stack here.
  std::optional<impl::stack> prev = std::nullopt;

  impl::y_combinate combinator = [&]() {
    if constexpr (is_void) {
      return combinate<tag::root>(impl::discard_t{}, std::move(fun));
    } else {
      return combinate<tag::root>(&result, std::move(fun));
    }
  }();

  if (!worker) {
    LF_LOG("Sync wait from non-worker thread");
    impl::tls::thread_stack.construct();
    impl::tls::has_stack = true;
  } else {
    LF_LOG("Sync wait from worker thread");
    prev.emplace();                        // Default construct.
    swap(*prev, *impl::tls::thread_stack); // ADL call.
  }

  // This makes a coroutine which can only be destroyed at final_suspend
  // hence, no exceptions until it has run.
  impl::quasi_awaitable await = std::move(combinator)(std::forward<Args>(args)...);

  [&]() noexcept {
    //
    await.prom->set_root_notify(&notifier);
    auto *handle = std::bit_cast<submit_handle>(static_cast<impl::frame *>(await.prom));

    impl::ignore_t{} = impl::tls::thread_stack->release();

    if (!worker) {
      impl::tls::thread_stack.destroy();
      impl::tls::has_stack = false;
    } else {
      swap(*prev, *impl::tls::thread_stack);
    }

    typename intrusive_list<submit_handle>::node node{handle};

    std::forward<Sch>(sch).schedule(&node);
    notifier.sem.acquire();
  }();

  if (notifier.m_eptr) {
    std::rethrow_exception(std::move(notifier.m_eptr));
  }

  if constexpr (!is_void) {
    return *std::move(result);
  }
}

} // namespace core

} // namespace lf

#endif /* AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A */
