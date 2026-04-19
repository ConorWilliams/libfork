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

// TODO: allocator aware! -> IDEA embed in frame/state

struct get_frame_t {};

/**
 * @brief Tag awaited inside the root body to install a type-erased shared_ptr
 *        keep-alive into the promise.
 *
 * The keep-alive is moved out of the frame by final_awaiter before the frame
 * is destroyed, so the receiver_state (which owns the buffer the frame lives
 * in) is kept alive until *after* frame teardown completes.
 */
struct set_keep_alive_t {
  std::shared_ptr<void> ptr;
};

template <typename Checkpoint>
struct root_task {
  struct promise_type {

    frame_type<Checkpoint> frame{Checkpoint{}};

    /// Owns a ref to the receiver_state hosting this coroutine's buffer.
    /// Moved out by `final_awaiter::await_suspend` before frame destruction.
    std::shared_ptr<void> keep_alive;

    /**
     * @brief Placement operator new: place the coroutine frame in the
     *        receiver_state's embedded buffer.
     *
     * Deduces R and Stoppable from the first coroutine argument (the
     * `std::shared_ptr<receiver_state<R, Stoppable>>` passed to `root_pkg`).
     */
    template <typename R, bool Stoppable, typename... CoroArgs>
    static auto operator new(std::size_t size,
                             std::shared_ptr<receiver_state<R, Stoppable>> const &recv,
                             CoroArgs const &.../*unused*/) noexcept -> void * {
      LF_ASSUME(recv != nullptr);
      LF_ASSUME(size <= receiver_state<R, Stoppable>::buffer_size);
      return recv->buffer();
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

    struct set_keep_alive_awaitable {
      set_keep_alive_t tag;
      promise_type *self;
      constexpr auto await_ready() const noexcept -> bool { return true; }
      constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
      constexpr void await_resume() noexcept { self->keep_alive = std::move(tag.ptr); }
    };

    constexpr auto await_transform(set_keep_alive_t tag) noexcept -> set_keep_alive_awaitable {
      return {.tag = std::move(tag), .self = this};
    }

    constexpr auto get_return_object() noexcept -> root_task { return {.promise = this}; }

    constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    /**
     * @brief Custom final_suspend.
     *
     * The root task's coroutine frame lives inside the receiver_state's
     * embedded buffer.  We must ensure the receiver_state (which owns that
     * buffer) outlives the frame teardown itself.  The strategy:
     *
     *   1. Move the type-erased keep-alive shared_ptr out of the promise
     *      into a local on the host stack.  After this the promise no
     *      longer holds a ref.
     *   2. Call `h.destroy()`.  This runs parameter + promise destructors
     *      and then our no-op `operator delete`.  No frame-memory access
     *      occurs after this point.
     *   3. Return from `await_suspend`.  As we unwind, the stack-local
     *      shared_ptr's destructor runs; if its ref was the last, it
     *      destroys the receiver_state (and releases the buffer memory)
     *      cleanly — we are no longer executing inside the buffer.
     *
     * Destroying a coroutine from within its own final_awaiter::await_suspend
     * is a well-known idiom: by the time await_suspend is called the body
     * has completed, so there is nothing else for the frame to do.
     */
    struct final_awaiter {
      constexpr auto await_ready() const noexcept -> bool { return false; }
      void await_suspend(std::coroutine_handle<promise_type> h) const noexcept {
        // Step 1: move keep-alive onto our stack.
        std::shared_ptr<void> local = std::move(h.promise().keep_alive);
        // Step 2: destroy the frame (promise + parameter destructors, no-op delete).
        h.destroy();
        // Step 3: `local` goes out of scope on return — possibly frees receiver_state.
      }
      constexpr void await_resume() const noexcept {}
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
root_pkg(std::shared_ptr<receiver_state<R, Stoppable>> recv, Fn fn, Args... args)
    -> root_task<checkpoint_t<Context>> {

  // This should be resumed on a valid context.
  LF_ASSUME(thread_local_context<Context> != nullptr);

  using checkpoint = checkpoint_t<Context>;

  // Install the keep-alive shared_ptr into the promise before anything else.
  // The type-erased std::shared_ptr<void> shares ownership with `recv`, so
  // even after `recv` (the coroutine parameter) is destroyed during frame
  // teardown the receiver_state stays alive until final_awaiter drops the
  // keep-alive on the host stack.
  co_await set_keep_alive_t{std::shared_ptr<void>{recv}};

  // This is a pointer to the current root_task's frame
  frame_type<checkpoint> *root = not_null(co_await get_frame_t{});

  // Now we do a manual "call" invocation.

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
    get(key(), *recv).set_exception(std::current_exception());
    goto cleanup;
  }

  LF_ASSUME(child != nullptr);

  // Propagate parent/stop info to child
  child->frame.parent = root;
  child->frame.stop_token = root->stop_token;

  LF_ASSUME(child->frame.kind == category::call);

  if constexpr (!std::is_void_v<async_result_t<Fn, Context, Args...>>) {
    child->return_address = get(key(), *recv).return_value_address();
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
      get(key(), *recv).set_exception(extract_exception(root));
    }
  }

cleanup:
  // Now do that which we would otherwise do at a final suspend.
  // Notify the receiver that the task is done.
  get(key(), *recv).notify_ready();

  LF_ASSUME(root->steals == 0);
  LF_ASSUME(root->joins == k_u16_max);
  LF_ASSUME(root->exception_bit == 0);

  co_return;
}

} // namespace lf
