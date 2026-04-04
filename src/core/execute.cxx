
module;
#include "libfork/__impl/assume.hpp"
export module libfork.core:execute;

import std;

import :thread_locals;
import :concepts;
import :handles;
import :utility;

namespace lf {

// TODO: unify exception higerarchy

/**
 * @brief Bind this thread to a context and execute the scheduled tasks on that context/thread.
 *
 * This should not be called from a thread already bound to a context, once this call returns
 * the thread is unbound from the context.
 */
export template <worker_context Context>
constexpr void execute(Context &context, sched_handle<Context> handle) {

  if (thread_context<Context> != nullptr) {
    LF_THROW(std::runtime_error{"execute called from within a worker thread!"});
  }

  thread_context<Context> = std::addressof(context);

  defer _ = [] noexcept -> void {
    thread_context<Context> = nullptr;
  };

  auto *frame = static_cast<frame_type<checkpoint_t<Context>> *>(get(key(), handle));

  frame->handle().resume();
}

} // namespace lf
