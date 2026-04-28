module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
export module libfork.core:final_suspend;

import std;

import libfork.utils;

import :concepts_context;
import :frame;
import :handles;
import :thread_locals;

namespace lf {

template <typename T = void>
using coro = std::coroutine_handle<T>;

template <worker_context Context>
using frame_t = frame_type<checkpoint_t<Context>>;

// =============== Extract exception =============== //

/**
 * @brief Pull an exception out of a frame and clean-up the union/allocation.
 */
template <typename Checkpoint>
[[nodiscard]]
constexpr auto extract_exception(frame_type<Checkpoint> *frame) noexcept -> std::exception_ptr {

  LF_ASSUME(frame->exception_bit); // Should only be called if an exception was thrown.

  // Local copy
  std::exception_ptr except = std::move(*frame->except);

  // Should have been set by stash_current_exception
  LF_ASSUME(except != nullptr);

  // Clean-up exception state
  frame->exception_bit = 0;
  frame->except.destroy();

  return except; // NRVO
}

// =============== Final =============== //

/**
 * @brief The full final suspend logic.
 *
 * The final suspend logic is fully expressed in this function in brief:
 *
 * - Try to resume parent if a call.
 * - Try to resume parent if a fork with no stealing.
 * - Try to resume a stolen forked task if last to complete.
 *
 * This function also handles cancellation (of the parent) by iteratively
 * climbing up the parent chain.
 *
 * This function is split and repeated as two separate functions to allow the
 * hot-path code to be inlined more easily into the final suspend.
 */
template <worker_context Context>
[[nodiscard]]
constexpr auto final_suspend_full(Context &context, frame_t<Context> *frame) noexcept -> coro<> {
  for (;;) {
    // Validate final state
    LF_ASSUME(frame);
    LF_ASSUME(frame->kind != category::root);
    LF_ASSUME(frame->steals == 0);
    LF_ASSUME(frame->joins == k_u16_max);
    LF_ASSUME(frame->exception_bit == 0);

    // Local copies (before we destroy frame)
    category const kind = frame->kind;

    frame_t<Context> *parent = not_null(frame->parent);

    // Before resuming the next (or exiting) we should clean-up the current frame.
    // Can't use frame from this point onwards
    frame->handle().destroy();

    if (kind == category::call) {
      return parent->handle();
    }

    // Given we are not a call we must be a fork hence, our
    // parent can't be a root as they can only call.
    LF_ASSUME(kind == category::fork);
    LF_ASSUME(parent->kind != category::root);

    if (steal_handle<Context> last_pushed = context.pop()) {
      // No-one stole continuation, we are the exclusive owner of parent -> just keep ripping!
      LF_ASSUME(last_pushed == steal_handle<Context>{key(), parent});
      // This is not a join point so no state (i.e. counters) is guaranteed.
      return parent->handle();
    }

    // An owner is a worker who:
    //
    // - Created the task.
    // - OR had the task submitted to them.
    // - OR won the task at a join.
    //
    // An owner of a task owns the stack the task is on.
    //
    // As the worker who completed the child task this thread owns the stack the child task was on.
    //
    // Either:
    //
    // 1. The parent is on the same stack as the child.
    // 2. OR the parent is on a different stack to the child.
    //
    // Case (1) implies: we owned the parent; forked the child task; then the parent was then stolen.
    // Case (2) implies: we stole the parent task; then forked the child; then the parent was stolen.
    //
    // Case (2) implies that our stack is empty.

    // As soon as we do the `fetch_sub` below the parent task is no longer safe
    // to access as it may be resumed and then destroyed by another thread. Hence
    // we must make copies on-the-stack of any data we may need if we lose the
    // join race.
    bool const owner = parent->stack_ckpt == context.stack().checkpoint();

    // As soon as we do the fetch_sub (if we lose) someone may acquire
    // the stack so we must prepare it for release now.
    auto release_key = context.stack().prepare_release();

    // Register with parent we have completed this child task.
    if (parent->atomic_joins().fetch_sub(1, std::memory_order_release) == 1) {
      // Parent has reached join and we are the last child task to complete. We
      // are the exclusive owner of the parent and therefore, we must continue
      // parent. As we won the race, acquire all writes before resuming.
      std::atomic_thread_fence(std::memory_order_acquire);

      if (!owner) {
        // In case of scenario (2) we must acquire the parent's stack.
        context.stack().acquire(std::as_const(parent->stack_ckpt));
      }

      // Must reset parent's control block before resuming parent.
      parent->reset_counters();

      if (parent->stop_requested()) [[unlikely]] {
        // Don't resume if stopped
        if constexpr (LF_COMPILER_EXCEPTIONS) {
          if (parent->exception_bit) [[unlikely]] {
            std::ignore = extract_exception(parent);
          }
        }
        frame = parent;
        continue;
      }

      return parent->handle();
    }

    if (owner) {
      // We were unable to resume the parent and we were its owner, as the
      // resuming thread will take ownership of the parent's we must give it up.
      context.stack().release(std::move(release_key));
    }

    // We did not win the join-race, we cannot dereference the parent pointer now
    // as the frame may now be freed by the winner. Parent has not reached join
    // or we are not the last child to complete. We are now out of jobs, we must
    // yield to the executor.

    // Else, case (2), our stack has no allocations on it, it may be used later.
    return std::noop_coroutine();
  }
}

template <worker_context Context>
[[nodiscard]]
constexpr auto final_suspend_trailing(Context &context, frame_t<Context> *parent) noexcept -> coro<> {

  bool const owner = parent->stack_ckpt == context.stack().checkpoint();

  auto release_key = context.stack().prepare_release();

  if (parent->atomic_joins().fetch_sub(1, std::memory_order_release) == 1) {

    std::atomic_thread_fence(std::memory_order_acquire);

    if (!owner) {
      context.stack().acquire(std::as_const(parent->stack_ckpt));
    }

    parent->reset_counters();

    if (parent->stop_requested()) [[unlikely]] {
      if constexpr (LF_COMPILER_EXCEPTIONS) {
        if (parent->exception_bit) [[unlikely]] {
          std::ignore = extract_exception(parent);
        }
      }
      return final_suspend_full<Context>(context, parent);
    }

    return parent->handle();
  }

  if (owner) {
    context.stack().release(std::move(release_key));
  }

  return std::noop_coroutine();
}

template <worker_context Context>
[[nodiscard]]
constexpr auto final_suspend_leading(frame_t<Context> *frame) noexcept -> coro<> {

  LF_ASSUME(frame);
  LF_ASSUME(frame->steals == 0);
  LF_ASSUME(frame->joins == k_u16_max);
  LF_ASSUME(frame->exception_bit == 0);

  category const kind = frame->kind;

  frame_t<Context> *parent = not_null(frame->parent);

  // The frame may have been allocated on a different worker's stack and then
  // posted here via `switch_awaitable` / `hop_to`.  In that case the source
  // released its stack and we must acquire it before popping the frame's
  // allocation, otherwise `operator delete` would pop from this worker's TLS
  // stack which never had the frame pushed onto it.  In the common case
  // (frame was allocated here) the checkpoints match and this is a no-op.
  if (auto frame_ckpt = frame->stack_ckpt; frame_ckpt != get_tls_stack<Context>().checkpoint()) {
    get_tls_stack<Context>().acquire(std::move(frame_ckpt));
  }

  frame->handle().destroy();

  if (kind == category::call) {
    return parent->handle();
  }

  LF_ASSUME(kind == category::fork);

  Context &context = get_tls_context<Context>();

  if (steal_handle<Context> last_pushed = context.pop()) {
    LF_ASSUME(last_pushed == steal_handle<Context>{key(), parent});
    return parent->handle();
  }

  return final_suspend_trailing<Context>(context, parent);
}

} // namespace lf
