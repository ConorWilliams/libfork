
module;
#include "libfork/__impl/assume.hpp"
export module libfork.core:execute;

import std;

import :frame;
import :thread_locals;
import :concepts_context;
import :handles;
import :exception;

namespace lf {

export struct execute_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "execute called from within a worker thread!";
  }
};

/**
 * @brief Bind this thread to a context and execute the scheduled tasks on that context/thread.
 *
 * This should not be called from a thread already bound to a context, once this call returns
 * the thread is unbound from the context.

 * The handle must not be null.
 */
export template <worker_context Context>
constexpr void execute(Context &context, sched_handle<Context> handle) {

  LF_ASSUME(handle);

  if (thread_local_context<Context> != nullptr) {
    LF_THROW(execute_error{});
  }

  thread_local_context<Context> = std::addressof(context);

  defer _ = [] static noexcept -> void {
    thread_local_context<Context> = nullptr;
  };

  auto *frame = static_cast<frame_type<checkpoint_t<Context>> *>(get(key(), handle));

  frame->handle().resume();
}

export struct steal_overflow_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "a single task has been stolen 65,535 times";
  }
};

/**
 * @brief Consume a steal handle, marks it as stolen and retuns the handle of the stolen task.
 *
 * The current thread must resume the handle.
 *
 * May throw `steal_overflow_error` if the task has been stolen enough tines to
 * overflow the steal counter.
 */
template <worker_context Context>
constexpr auto consume(steal_handle<Context> handle) -> std::coroutine_handle<> {

  LF_ASSUME(handle);

  auto *frame = static_cast<frame_type<checkpoint_t<Context>> *>(get(key(), handle));

  if (frame->steals == k_u16_max) {
    LF_THROW(steal_overflow_error{});
  }

  frame->steals += 1;

  return frame->handle();
}

/**
 * @brief Bind this thread to a context and execute the scheduled tasks on that context/thread.
 *
 * This should not be called from a thread already bound to a context, once this call returns
 * the thread is unbound from the context.
 *
 * The handle must not be null.
 */
export template <worker_context Context>
constexpr void execute(Context &context, steal_handle<Context> handle) {

  LF_ASSUME(handle);

  if (thread_local_context<Context> != nullptr) {
    LF_THROW(execute_error{});
  }

  thread_local_context<Context> = std::addressof(context);

  defer _ = [] static noexcept -> void {
    thread_local_context<Context> = nullptr;
  };

  consume(handle).resume();
}

} // namespace lf
