#ifndef AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A
#define AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>         // for bit_cast
#include <exception>   // for exception, rethrow_exception
#include <memory>      // for make_shared, shared_ptr
#include <optional>    // for optional
#include <semaphore>   // for binary_semaphore
#include <type_traits> // for is_trivially_destructible_v
#include <utility>     // for forward, exchange

#include "libfork/core/defer.hpp"                // for LF_DEFER
#include "libfork/core/eventually.hpp"           // for try_eventually
#include "libfork/core/exceptions.hpp"           // for schedule_in_worker
#include "libfork/core/ext/handles.hpp"          // for submit_node_t, submit_t
#include "libfork/core/ext/tls.hpp"              // for has_stack, thread_stack, has_context
#include "libfork/core/first_arg.hpp"            // for async_function_object
#include "libfork/core/impl/combinate.hpp"       // for quasi_awaitable, y_combinate
#include "libfork/core/impl/manual_lifetime.hpp" // for manual_lifetime
#include "libfork/core/impl/stack.hpp"           // for stack
#include "libfork/core/impl/utility.hpp"
#include "libfork/core/invocable.hpp" // for async_result_t, rootable, ignore_t
#include "libfork/core/macro.hpp"     // for LF_THROW, LF_CLANG_TLS_NOINLINE
#include "libfork/core/scheduler.hpp" // for scheduler
#include "libfork/core/tag.hpp"       // for tag, none
#include "libfork/core/task.hpp"      // for returnable

/**
 * @file sync_wait.hpp
 *
 * @brief Functionally to enter coroutines from a non-worker thread.
 */

namespace lf {

namespace impl {

/**
 * @brief State of a future.
 */
enum class future_state {
  /**
   * @brief Wait has not been called.
   */
  no_wait,
  /**
   * @brief The result is ready.
   */
  ready,
  /**
   * @brief The result has been retrievd.
   */
  retrievd,
};

/**
 * @brief The shared state of a future.
 */
template <typename R>
struct future_shared_state : try_eventually<R> {
  /**
   * @brief Inherit assignment operators.
   */
  using try_eventually<R>::operator=;

  static_assert(std::is_trivially_destructible_v<submit_node_t>);
  /**
   * @brief The submit handle will point to this.
   *
   * We never call `.destroy()` on this but that is ok by the above `static_assert`.
   */
  manual_lifetime<submit_node_t> node;
  /**
   * @brief The root task's notification semaphore.
   */
  std::binary_semaphore sem{0};
  /**
   * @brief The state of the future.
   */
  future_state status = future_state::no_wait;
};

/**
 * @brief An ``std::shared_pointer`` to a shared future state.
 */
template <typename R>
using future_shared_state_ptr = std::shared_ptr<future_shared_state<R>>;

} // namespace impl

inline namespace core {

/**
 * @brief Thrown when a future has no shared state.
 */
struct broken_future : std::exception {
  /**
   * @brief A diagnostic message.
   */
  auto what() const noexcept -> char const * override { return "Broken future, no shared state!"; }
};

/**
 * @brief Thrown when `.get()` is called more than once on a future.
 */
struct empty_future : std::exception {
  /**
   * @brief A diagnostic message.
   */
  auto what() const noexcept -> char const * override { return "future::get() called more than once!"; }
};

/**
 * @brief A future is a handle to the result of an asynchronous operation.
 */
template <returnable R>
class future {

  using enum impl::future_state;

  /**
   * @brief The other half of the promise-future pair.
   */
  impl::future_shared_state_ptr<R> m_heap;

  template <scheduler Sch, async_function_object F, class... Args>
    requires rootable<F, Args...>
  friend auto schedule(Sch &&sch, F &&fun, Args &&...args) -> future<async_result_t<F, Args...>>;

// Work-around: https://github.com/llvm/llvm-project/issues/63536
#if defined(__clang__)
  #if defined(__apple_build_version__)
    #if __clang_major__ == 17
 public
    #endif
  #elif __clang_major__ == 16
 public
  #endif
#endif
  /**
   * @brief Construct a new future object storing the shared state.
   */
  explicit future(impl::future_shared_state_ptr<R> &&heap) noexcept : m_heap{std::move(heap)} {
    static_assert(std::is_nothrow_move_constructible_v<impl::future_shared_state_ptr<R>>);
  }

 public:
  /**
   * @brief Move construct a new future.
   */
  future(future &&other) noexcept = default;
  /**
   * @brief Futures are not copyable.
   */
  future(future const &other) = delete;
  /**
   * @brief Move assign to a future.
   */
  auto operator=(future &&other) noexcept -> future & = default;
  /**
   * @brief Futures are not copy assignable.
   */
  auto operator=(future const &other) -> future & = delete;
  /**
   * @brief Wait (__block__) until the future completes if it has a shared state.
   */
  ~future() noexcept {
    if (valid() && m_heap->status == no_wait) {
      m_heap->sem.acquire();
    }
  }
  /**
   * @brief Test if the future has a shared state.
   */
  auto valid() const noexcept -> bool { return m_heap != nullptr; }
  /**
   * @brief Detach the shared state from this future.
   *
   * Following this operation the destructor is guaranteed to not block.
   */
  void detach() noexcept { std::exchange(m_heap, nullptr); }
  /**
   * @brief Wait (__block__) for the future to complete.
   */
  void wait() {

    if (!valid()) {
      LF_THROW(broken_future{});
    }

    if (m_heap->status == no_wait) {
      m_heap->sem.acquire();
      m_heap->status = ready;
    }
  }
  /**
   * @brief Wait (__block__) for the result to complete and then return it.
   *
   * If the task completed with an exception then that exception will be rethrown. If
   * the future has no shared state then a `lf::core::future_error` will be thrown.
   */
  auto get() -> R {

    wait();

    if (m_heap->status == retrievd) {
      LF_THROW(empty_future{});
    }

    m_heap->status = retrievd;

    if (m_heap->has_exception()) {
      std::rethrow_exception(std::move(*m_heap).exception());
    }

    if constexpr (!std::is_void_v<R>) {
      return *std::move(*m_heap);
    }
  }
};

/**
 * @brief Thrown when a worker thread attempts to call `lf::core::schedule`.
 */
struct schedule_in_worker : std::exception {
  /**
   * @brief A diagnostic message.
   */
  auto what() const noexcept -> char const * override { return "schedule(...) called from a worker thread!"; }
};

/**
 * @brief Schedule execution of `fun` on `sch` and return a `lf::core::future` to the result.
 *
 * This will build a task from `fun` and dispatch it to `sch` via its `schedule` method. If `schedule` is
 * called by a worker thread (which are never allowed to block) then `lf::core::schedule_in_worker` will be
 * thrown.
 */
template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
LF_CLANG_TLS_NOINLINE auto
schedule(Sch &&sch, F &&fun, Args &&...args) -> future<async_result_t<F, Args...>> {
  //
  if (impl::tls::has_stack || impl::tls::has_context) {
    throw schedule_in_worker{};
  }

  // Initialize the non-worker's stack.
  impl::tls::thread_stack.construct();
  impl::tls::has_stack = true;

  // Clean up the stack on exit.
  LF_DEFER {
    impl::tls::thread_stack.destroy();
    impl::tls::has_stack = false;
  };

  auto share_state = std::make_shared<impl::future_shared_state<async_result_t<F, Args...>>>();

  // Build a combinator, copies heap shared_ptr.
  impl::y_combinate combinator = combinate<tag::root, modifier::none>(share_state, std::forward<F>(fun));
  // This allocates a coroutine on this threads stack.
  impl::quasi_awaitable await = std::move(combinator)(std::forward<Args>(args)...);
  // Set the root semaphore.
  await->set_root_sem(&share_state->sem);

  // If this throws then `await` will clean up the coroutine.
  impl::ignore_t{} = impl::tls::thread_stack->release();

  // We will pass a pointer to this to .schedule()
  share_state->node.construct(std::bit_cast<impl::submit_t *>(await.get()));

  // Schedule upholds the strong exception guarantee hence, if it throws `await` cleans up.
  std::forward<Sch>(sch).schedule(share_state->node.data());
  // If -^ didn't throw then we release ownership of the coroutine, it will be cleaned up by the worker.
  impl::ignore_t{} = await.release();

  return future<async_result_t<F, Args...>>{std::move(share_state)}; // Shared state ownership transferred.
}

/**
 * @brief Schedule execution of `fun` on `sch` and wait (__block__) until the task is complete.
 *
 * This is the primary entry point from the synchronous to the asynchronous world. A typical libfork program
 * is expected to make a call from `main` into a scheduler/runtime by scheduling a single root-task with this
 * function.
 *
 * This makes the appropriate call to `lf::core::schedule` and calls `get` on the returned `lf::core::future`.
 */
template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
auto sync_wait(Sch &&sch, F &&fun, Args &&...args) -> async_result_t<F, Args...> {
  return schedule(std::forward<Sch>(sch), std::forward<F>(fun), std::forward<Args>(args)...).get();
}

/**
 * @brief Schedule execution of `fun` on `sch` and detach the future.
 *
 * This is the secondary entry point from the synchronous to the asynchronous world. Similar to `sync_wait`
 * but calls `detach` on the returned `lf::core::future`.
 *
 * __Note:__ Many schedulers (like `lf::lazy_pool` and `lf::busy_pool`) require all submitted work to
 * (including detached work) to complete before they are destructed.
 */
template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
auto detach(Sch &&sch, F &&fun, Args &&...args) -> void {
  return schedule(std::forward<Sch>(sch), std::forward<F>(fun), std::forward<Args>(args)...).detach();
}

} // namespace core

} // namespace lf

#endif /* AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A */
