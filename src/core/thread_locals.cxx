export module libfork.core:thread_locals;

import libfork.utils;

import :concepts_context;

namespace lf {

/**
 * @brief Thread-local pointer to the current worker context.
 */
export template <worker_context Context>
constinit inline thread_local Context *thread_local_context = nullptr;

// TODO: implictaions of thread local on constexpr

/**
 * @brief A getter for the current worker context, checks for null in debug.
 */
template <worker_context Context>
constexpr auto get_tls_context() noexcept -> Context & {
  return *not_null(thread_local_context<Context>);
}

template <worker_context Context>
constexpr auto get_tls_stack() noexcept -> stack_t<Context> & {
  return get_tls_context<Context>().stack();
}

} // namespace lf
