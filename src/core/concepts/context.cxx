export module libfork.core:concepts_context;

import std;

import libfork.utils;

import :concepts_stack;
import :handles;

namespace lf {

template <typename T>
concept ref_to_worker_stack = std::is_lvalue_reference_v<T> && worker_stack<std::remove_reference_t<T>>;

/**
 * @brief Specifies that a type acts as a LIFO stack over U.
 */
export template <typename T, typename U>
concept lifo_stack = plain_object<T> && requires (T context, U val) {
  { context.push(val) } -> std::same_as<void>;
  { context.pop() } noexcept -> std::same_as<U>;
};

/**
 * @brief Defines the API for a libfork compatible worker context.
 *
 * This requires that `T` is an object type and supports the following operations:
 *
 * - Push/pop a steal handle onto the context in a LIFO manner.
 * - Have a `worker_stack` that can be accessed via `stack()`.
 */
export template <typename T>
concept worker_context = lifo_stack<T, steal_handle<T>> && requires (T context) {
  { context.stack() } noexcept -> ref_to_worker_stack;
};

/**
 * @brief Fetch the stack type of a worker context `T`.
 */
export template <worker_context T>
using stack_t = std::remove_reference_t<decltype(std::declval<T &>().stack())>;

/**
 * @brief Fetch the checkpoint type of a worker context `T`.
 */
export template <worker_context T>
using checkpoint_t = decltype(std::declval<stack_t<T> &>().checkpoint());

} // namespace lf
