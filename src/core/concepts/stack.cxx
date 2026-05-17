export module libfork.core:concepts_stack;

import std;

import libfork.utils;

namespace lf {

template <typename T>
  requires std::is_object_v<T>
consteval auto constify(T &&x) noexcept -> std::add_const_t<T> &;

/**
 * @brief Defines the API for a libfork compatible stack.
 *
 * - After construction `this` is empty and push is valid.
 * - Pop is valid provided the FILO order is respected.
 * - Push produces pointers aligned to __STDCPP_DEFAULT_NEW_ALIGNMENT__.
 * - Destruction is expected to only occur when the stack is empty.
 * - Result of `.checkpoint()` is expected to:
 *     - Be "cheap to copy".
 *     - Have a null state (default constructed) that only compares equal to itself.
 *     - Is allowed to return null if push has never been called.
 *     - Compare equal if and only if they are both null or they allocate from the same stack.
 *     - Have no preconditions about when it's called.
 * - Prepare release puts the stack into a state which another thread can acquire it.
 * - Release detaches the current stack and leaves `this` empty.
 *     - This may be called concurrently with acquire
 * - Acquire attaches to the stack that the checkpoint came from:
 *     - It is only called the stack is empty.
 *     - It is only called with a checkpoint not equal to the current checkpoint.
 *     - It is called after prepare release (and no other functions in between)
 *
 * Fast-path operations: empty, push, pop, checkpoint
 * Slow-path operations: release, acquire
 */
export template <typename T>
concept worker_stack = plain_object<T> && requires (T stack, std::size_t n, void *ptr) {
  { stack.push(n) } -> std::same_as<void *>;
  { stack.pop(ptr, n) } noexcept -> std::same_as<void>;
  { stack.checkpoint() } noexcept -> std::regular;
  { stack.prepare_release() } noexcept -> std::movable;
  { stack.release(stack.prepare_release()) } noexcept -> std::same_as<void>;
  { stack.acquire(constify(stack.checkpoint())) } noexcept -> std::same_as<void>;
};

// TODO: Allocator aware stack

// export template <typename T>
// concept aa_worker_stack = worker_stack<T> && true;

} // namespace lf
