module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:root;

import std;

import libfork.utils;

import :concepts_context;
import :concepts_invocable;
import :frame;
import :promise;
import :receiver;
import :thread_locals;
import :task;

namespace lf {

/**
 * @brief Thrown if the root coroutine frame is too large for the embedded buffer.
 */
export struct root_alloc_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "root coroutine frame exceeds receiver_state buffer size";
  }
};

struct get_frame_t {};

template <typename Checkpoint>
struct root_task {
  struct promise_type {

    frame_type<Checkpoint> frame{Checkpoint{}};

    /// Owns a ref to the receiver_state hosting this frame's buffer.
    std::shared_ptr<void> keep_alive;

    template <typename R, bool Stoppable, typename... Args>
    constexpr explicit promise_type(state_handle<R, Stoppable> const &recv, Args const &...) noexcept
        : keep_alive(recv) {}

    template <typename R, bool Stoppable, typename... Args>
    static auto
    operator new(std::size_t size, state_handle<R, Stoppable> const &recv, Args const &...) -> void * {

      LF_ASSUME(recv != nullptr);

      if (size > recv->buffer.size()) {
        LF_THROW(root_alloc_error{});
      }

      return recv->buffer.data();
    }

    /// No-op: the buffer is owned by the receiver_state, not the frame.
    static auto operator delete(void * /*ptr*/, std::size_t /*size*/) noexcept -> void {}

    struct frame_awaitable : std::suspend_never {
      frame_type<Checkpoint> *frame;
      [[nodiscard]]
      constexpr auto await_resume() const noexcept -> frame_type<Checkpoint> * {
        return frame;
      }
    };

    constexpr auto await_transform([[maybe_unused]] get_frame_t tag) noexcept -> frame_awaitable {
      return {.frame = &frame};
    }

    struct call_awaitable : std::suspend_always {
      frame_type<Checkpoint> *child;
      constexpr auto await_suspend([[maybe_unused]] coro<promise_type> root) const noexcept -> coro<> {
        return child->handle();
      }
    };

    constexpr auto await_transform(frame_type<Checkpoint> *child) noexcept -> call_awaitable {
      return {.child = child};
    }

    constexpr auto get_return_object() noexcept -> root_task { return {.promise = this}; }

    constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    /**
     * @brief Custom final_suspend.
     *
     * The root coroutine frame lives inside the receiver_state's embedded
     * buffer, so the receiver_state must outlive the frame teardown.
     *
     *   1. `std::exchange` the keep-alive shared_ptr into a local on the
     *      host stack, leaving the promise member null.
     *   2. `handle.destroy()` — runs parameter + promise destructors (including
     *      the now-null `keep_alive`) and our no-op `operator delete`.
     *      No frame-memory access occurs after the handle returns.
     *   3. On return, the stack-local `shared_ptr<void>` dies; if its ref
     *      was the last, it destroys the receiver_state cleanly — we are
     *      no longer executing inside the buffer.
     */
    struct final_awaiter : std::suspend_always {
      void await_suspend(std::coroutine_handle<promise_type> handle) const noexcept {
        std::shared_ptr<void> local = std::exchange(handle.promise().keep_alive, nullptr);
        LF_ASSUME(local != nullptr);
        handle.destroy();
        // `local` released here — possibly freeing receiver_state on return.
      }
    };

    constexpr static auto final_suspend() noexcept -> final_awaiter { return {}; }

    constexpr static void return_void() noexcept {}

    [[noreturn]]
    constexpr void unhandled_exception() noexcept {
      // Any exceptions escaping the root task are a bug.
      LF_UNREACHABLE();
    }
  };

  promise_type *promise;
};

template <worker_context Context, typename R, bool Stoppable, typename Fn, typename... Args>
  requires async_invocable_to<Fn, R, Context, Args...>
[[nodiscard]]
auto //
root_pkg(state_handle<R, Stoppable> recv, Fn fn, Args... args) -> root_task<checkpoint_t<Context>> {

  // This should be resumed on a valid context.
  LF_ASSUME(thread_local_context<Context> != nullptr);

  using checkpoint = checkpoint_t<Context>;

  // Pointer to this root_task's own frame.
  frame_type<checkpoint> *root = not_null(co_await get_frame_t{});

  // Manual "call" invocation of the user-supplied task.

  using result_type = async_result_t<Fn, Context, Args...>;
  using promise_type = promise_type<result_type, Context>;

  promise_type *child = nullptr;

  if (root->stop_requested()) {
    // The root task was cancelled before it even started, we can skip
    // straight to cleanup.
    goto cleanup;
  }

  LF_TRY {
    // Potentially throwing
    child = get(key(), ctx_invoke_t<Context>{}(std::move(fn), std::move(args)...));
  } LF_CATCH_ALL {
    recv->exception = std::current_exception();
    goto cleanup;
  }

  LF_ASSUME(child != nullptr);

  // Propagate parent/stop info to child
  child->frame.parent = root;
  child->frame.stop_token = root->stop_token;

  LF_ASSUME(child->frame.kind == category::call);

  if constexpr (!std::is_void_v<async_result_t<Fn, Context, Args...>>) {
    child->return_address = std::addressof(recv->return_value);
  }

  // Begin normal execution of the child task, it will clean itself
  // up (i.e. .destroy()) at the final suspend
  co_await &child->frame;

  // Now we have been resumed the child is done, it could have completed via:
  //
  // - Normal return
  // - Exception
  // - Cancellation
  //
  // We return any exception stashed unconditionally

  if constexpr (LF_COMPILER_EXCEPTIONS) {
    if (root->exception_bit) {
      // The child threw an exception, propagate it to the receiver.
      recv->exception = extract_exception(root);
    }
  }

cleanup:
  // Notify the receiver that the task is done.
  recv->ready.test_and_set();
  recv->ready.notify_one();

  LF_ASSUME(root->steals == 0);
  LF_ASSUME(root->joins == k_u16_max);
  LF_ASSUME(root->exception_bit == 0);

  co_return;
}

} // namespace lf
