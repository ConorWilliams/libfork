export module libfork.core:thread_locals;

import std;

import :concepts;
import :utility;

namespace lf {

/**
 * @brief Thread-local pointer to the current worker context.
 */
export template <worker_context Context>
constinit inline thread_local Context *thread_context = nullptr;

/**
 * @brief A getter for the current worker context, checks for null in debug.
 */
template <worker_context Context>
constexpr auto get_context() noexcept -> Context * {
  return not_null(thread_context<Context>);
}

template <worker_context Context>
constexpr auto get_allocator() noexcept -> allocator_t<Context> & {
  return get_context<Context>()->allocator();
}

} // namespace lf
