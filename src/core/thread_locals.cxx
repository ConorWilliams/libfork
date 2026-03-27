export module libfork.core:thread_locals;

import std;

import :concepts;

namespace lf {

/**
 * @brief Thread-local pointer to the current worker context.
 */
export template <worker_context Context>
constinit inline thread_local Context *thread_context = nullptr;

} // namespace lf
