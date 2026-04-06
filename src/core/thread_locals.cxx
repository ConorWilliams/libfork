export module libfork.core:thread_locals;

import std;

import libfork.utils;
import :concepts;

namespace lf {

/**
 * @brief Thread-local pointer to the current worker context.
 */
export template <worker_context Context>
constinit inline thread_local Context *thread_context = nullptr;

// TODO: return a reference, rename to get_tls_*

// TODO: implictaions of thread local on constexpr

/**
 * @brief A getter for the current worker context, checks for null in debug.
 */
template <worker_context Context>
constexpr auto get_context() noexcept -> Context * {
  return not_null(thread_context<Context>);
}

template <worker_context Context>
constexpr auto get_stack() noexcept -> stack_t<Context> & {
  return get_context<Context>()->stack();
}

} // namespace lf
