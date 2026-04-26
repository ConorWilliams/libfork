module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:awaitables;

import std;

import libfork.utils;

import :concepts_context;
import :frame;
import :handles;
import :task;
import :thread_locals;
import :final_suspend;

namespace lf {

// =============== Fork/Call =============== //

/**
 * @brief Call inside a catch block, stash current exception in `frame`.
 */
template <typename Checkpoint>
constexpr void stash_current_exception(frame_type<Checkpoint> *frame) noexcept {
  // No synchronization is done via exception_bit, hence we can use relaxed atomics
  // and rely on the usual fork/join synchronization to ensure memory ordering.
  if (frame->atomic_except().exchange(1, std::memory_order_relaxed) == 0) {

    frame->except.construct(std::current_exception());

    // Should have been called from inside a catch block
    LF_ASSUME(*frame->except != nullptr);
  }
}

/**
 * @brief In a separate function to allow it to be placed in cold block.
 */
template <typename T, typename Context>
constexpr void
destroy_child_stash_exception(frame_t<Context> *child, coro<promise_type<T, Context>> parent) noexcept {
  // Clean-up the child that will never be resumed.
  child->handle().destroy();
  // Stash in the parent's frame which will then be resumed.
  stash_current_exception(&parent.promise().frame);
}

/**
 * @brief Awaitable for forking/calling an async function.
 */
template <category Cat, worker_context Context>
struct async_awaitable : std::suspend_always {

  static_assert(Cat == category::call || Cat == category::fork, "Invalid category for awaitable");

  frame_t<Context> *child;

  template <typename T>
  constexpr auto
  await_suspend(this async_awaitable self, coro<promise_type<T, Context>> parent) noexcept -> coro<> {

    // TODO: test of having a dedicated is_stopped awaitable is quicker

    if (!self.child) [[unlikely]] {
      // Noop if an exception was thrown.
      return parent;
    }

    if (self.child->stop_requested()) [[unlikely]] {
      // Noop if stopped, must clean-up the child that will never be resumed.
      return self.child->handle().destroy(), parent;
    }

    // Propagate parent->child relationships
    self.child->parent = &parent.promise().frame;

    if constexpr (Cat == category::call) {
      // Should be the default
      LF_ASSUME(self.child->kind == category::call);
    } else {
      self.child->kind = Cat;
    }

    if constexpr (Cat == category::fork) {
      // It is critical to pass self by-value here, after the call to push()
      // the object `*this` may be destroyed, if passing by ref it would be
      // use-after-free to then access self in the following line to fetch the
      // handle.
      LF_TRY {
        get_tls_context<Context>().push(steal_handle<Context>{key(), &parent.promise().frame});
      } LF_CATCH_ALL {
        return destroy_child_stash_exception(self.child, parent), parent;
      }
    }

    return self.child->handle();
  }
};

// =============== Join =============== //

template <worker_context Context>
struct join_awaitable {

  frame_t<Context> *frame;

  constexpr auto await_ready(this join_awaitable self) noexcept -> bool {
    if (not_null(self.frame)->steals == 0) [[likely]] {
      if (self.frame->stop_requested()) [[unlikely]] {
        // Must unconditionally suspended if stopped
        return false;
      }
      // If no steals then we are the only owner of the parent and we are
      // ready to join. Therefore, no need to reset the control block.
      return true;
    }
    return false;
  }

  constexpr auto await_suspend(this join_awaitable self, std::coroutine_handle<> task) noexcept -> coro<> {
    // Currently   self.joins  = k_u16_max  - num_joined
    //
    // We set           joins  = self->joins - (k_u16_max - num_steals)
    //                         = num_steals - num_joined
    //
    // Hence               joined = k_u16_max - num_joined
    //         k_u16_max - joined = num_joined

    // Lemma:
    //
    //    If a thread is at a join and steals have occurred then the
    //    thread can never own the stack of the current frame.
    //
    // This is because threads follow the work-first principle, so for the
    // owner to be running this task it would have to have re-stolen it from a
    // thief. Which implies it would have run the final suspend of the child
    // that had it's continuation stolen, where it would have had to release
    // the stack, because the parent was at not at the join.

    LF_ASSUME(self.frame);

    std::uint32_t steals = self.frame->steals;
    std::uint32_t offset = k_u16_max - steals;
    std::uint32_t joined = self.frame->atomic_joins().fetch_sub(offset, std::memory_order_release);

    // If this was a stop:
    //
    // steals = 0, joins = k_u16_max then:
    //
    // steals = 0
    // offset = k_u16_max
    // joined = k_u16_max, (self.frame->joins is now 0)
    //
    // k_u16_max - joined = 0 = steals, hence win the if

    if (steals == k_u16_max - joined) {
      // We set joins after all children had completed therefore we can resume the task.
      // Need to acquire to ensure we see all writes by other threads to the result.
      std::atomic_thread_fence(std::memory_order_acquire);

      if (self.frame->stop_requested()) [[unlikely]] {
        return self.handle_stop();
      }

      // We must reset the control block and take the stack. We should never
      // own the stack at this point because we must have stolen the stack.
      self.take_stack();
      self.frame->reset_counters();
      return task;
    }
    // Someone else is responsible for running this task.

    // We cannot touch *this or dereference self as someone may have resumed already!
    // We cannot currently own this stack (checking would violate above).

    // If no explicit scheduling then we must have an empty WSQ as we stole this task.

    // If explicit scheduling then we may have tasks on our WSQ if we performed a self-steal
    // in a switch awaitable. In this case we can/must do another self-steal.

    // return try_self_stealing();

    return std::noop_coroutine();
  }

  constexpr void await_resume(this join_awaitable self) {
    // We should have been reset
    LF_ASSUME(self.frame->steals == 0);
    LF_ASSUME(self.frame->joins == k_u16_max);

    // Outside parallel regions so can touch non-atomically.
    //
    // A task that completes by responding to cancellation will drop any
    // exceptions however, a task may still throw exceptions even if cancelled.
    // Here we must rethrow even if cancelled because we can't re-suspend at
    // this point.
    if constexpr (LF_COMPILER_EXCEPTIONS) {
      if (self.frame->exception_bit) [[unlikely]] {
        self.rethrow_exception();
      }
    }

    LF_ASSUME(self.frame->exception_bit == 0);
  }

  constexpr auto take_stack(this join_awaitable self) noexcept -> void {
    stack_t<Context> &stack = get_tls_stack<Context>();
    LF_ASSUME(self.frame->stack_ckpt != stack.checkpoint());
    stack.acquire(std::as_const(self.frame->stack_ckpt));
  }

  [[nodiscard]]
  constexpr auto handle_stop(this join_awaitable self) noexcept -> coro<> {
    // Only need to take the stack if there were steals
    if (self.frame->steals > 0) {
      self.take_stack();
    }

    // We always need to reset the connters as we modified
    self.frame->reset_counters();

    // Drop any exceptions in the now-stopped task
    if constexpr (LF_COMPILER_EXCEPTIONS) {
      if (self.frame->exception_bit) [[unlikely]] {
        std::ignore = extract_exception(self.frame);
      }
    }

    return final_suspend_leading<Context>(self.frame);
  }

  [[noreturn]]
  constexpr void rethrow_exception(this join_awaitable self) {
    std::rethrow_exception(extract_exception(self.frame));
  }
};

} // namespace lf
