module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:root;

import std;

import :frame;
import :promise;
import :receiver;

namespace lf {

// TODO: allocator aware!

struct get_frame_t {};

template <typename Checkpoint>
struct root_task {
  struct promise_type {

    frame_type<Checkpoint> frame{Checkpoint{}};

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

    constexpr static auto final_suspend() noexcept -> std::suspend_never { return {}; }

    constexpr static void return_void() noexcept {}

    [[noreturn]]
    constexpr void unhandled_exception() noexcept {
      // Any exceptions escaping the root task are a bug.
      LF_UNREACHABLE();
    }
  };

  promise_type *promise;
};

template <worker_context Context, typename R, typename Fn, typename... Args>
  requires async_invocable_to<Fn, R, Context, Args...>
auto package_as_root(std::shared_ptr<receiver_state<R>> recv, Fn fn, Args... args)
    -> root_task<checkpoint_t<Context>> {

  // This should be resumed on a valid context.
  LF_ASSUME(thread_context<Context> != nullptr);

  using checkpoint = checkpoint_t<Context>;

  // This is a pointer to the current root_task's frame
  frame_type<checkpoint> *root = not_null(co_await get_frame_t{});

  // Now we do a manual "call" invocation.

  using result_type = async_result_t<Fn, Context, Args...>;
  using promise_type = promise_type<result_type, Context>;

  promise_type *child = nullptr;

  LF_TRY {
    // Potentially throwing
    child = get(key(), ctx_invoke_t<Context>{}(std::move(fn), std::move(args)...));
  } LF_CATCH_ALL {
    recv->m_exception = std::current_exception();
    goto cleanup;
  }

  LF_ASSUME(child != nullptr);

  // TODO: cancellation

  child->frame.parent.frame = root;
  child->frame.cancel = nullptr;

  LF_ASSUME(child->frame.kind == category::call);

  if constexpr (!std::is_void_v<async_result_t<Fn, Context, Args...>>) {
    child->return_address = std::addressof(recv->m_return_value);
  }

  // Begin normal execution of the child task, it will clean itself
  // up (i.e. .destroy()) at the final suspend
  co_await &child->frame;

  // Now we have been resumed the child is done, it could have completed via:
  //
  // - Normal return
  // - Exception
  // - Cancellation

  if constexpr (LF_COMPILER_EXCEPTIONS) {
    if (root->exception_bit) {
      // The child threw an exception, propagate it to the receiver.
      recv->m_exception = extract_exception(root);
    }
  }

cleanup:
  // Now to that which we would otherwise do at a final suspend.
  // Notify the receiver that the task is done.
  recv->m_ready.test_and_set();
  recv->m_ready.notify_one();
  co_return;
}

} // namespace lf
