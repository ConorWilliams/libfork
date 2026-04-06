
module;
#include "libfork/__impl/assume.hpp"
export module libfork.core:execute;

import std;

import libfork.utils;
import :frame;
import :thread_locals;
import :concepts_context;
import :handles;

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
 */
export template <worker_context Context>
constexpr void execute(Context &context, sched_handle<Context> handle) {

  if (thread_local_context<Context> != nullptr) {
    LF_THROW(execute_error{});
  }

  thread_local_context<Context> = std::addressof(context);

  defer _ = [] noexcept -> void {
    thread_local_context<Context> = nullptr;
  };

  auto *frame = static_cast<frame_type<checkpoint_t<Context>> *>(get(key(), handle));

  frame->handle().resume();
}

} // namespace lf
