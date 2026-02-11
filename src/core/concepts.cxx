module;
export module libfork.core:concepts;

import std;

namespace lf {

// ========== Specialization ========== //

template <typename T, template <typename...> typename Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

/**
 * @brief Test if `T` is a specialization of the template `Template`.
 */
export template <typename T, template <typename...> typename Template>
concept specialization_of = is_specialization_of<std::remove_cvref_t<T>, Template>::value;

// ========== Task constraint related concepts ========== //

// ==== Returnable

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` or a `std::movable` type.
 */
template <typename T>
concept returnable = std::is_void_v<T> || std::movable<T>;

// ==== Stack

template <typename T>
  requires std::is_object_v<T>
consteval auto constify(T &&x) noexcept -> std::add_const_t<T> &;

/**
 * @brief Defines the API for a libfork compatible stack allocator.
 *
 * - After construction `this` is in the empty state and push is valid.
 * - Pop is valid provided the FILO order is respected.
 * - Destruction is expected to only occur when the stack is empty.
 * - Result of `.checkpoint()` is expected to be "cheap to copy".
 * - Release detaches the current stack and leaves `this` in the empty state.
 * - Acquire attaches to the stack that the checkpoint came from:
 *     - This is a noop if the checkpoint is from the current stack.
 *     - Otherwise `this` is empty.
 *
 * Fast-path operations: empty, push, pop, checkpoint
 * Slow-path operations: release, resume
 */
export template <typename T>
concept stack_allocator = std::is_object_v<T> && requires (T alloc, std::size_t n, void *ptr) {
  // { alloc.empty() } noexcept -> std::same_as<bool>;
  { alloc.push(n) } -> std::same_as<void *>;
  { alloc.pop(ptr, n) } noexcept -> std::same_as<void>;
  { alloc.checkpoint() } noexcept -> std::semiregular;
  { alloc.release() } noexcept -> std::same_as<void>;
  { alloc.acquire(constify(alloc.checkpoint())) } noexcept -> std::same_as<void>;
};

/**
 * @brief Fetch the checkpoint type of a stack allocator `T`.
 */
template <stack_allocator T>
using checkpoint_t = decltype(std::declval<T &>().checkpoint());

// ==== Context

export template <typename T>
class frame_handle;

template <typename T>
concept ref_to_stack_allocator = std::is_lvalue_reference_v<T> && stack_allocator<std::remove_reference_t<T>>;

/**
 * @brief Defines the API for a libfork compatible worker context.
 *
 * This requires that `T` is an object type and supports the following operations:
 *
 * - Push/pop a frame handle onto the context in a LIFO manner.
 * - Have a `stack_allocator` that can be accessed via `alloc()`.
 */
export template <typename T>
concept worker_context = std::is_object_v<T> && requires (T ctx, frame_handle<T> handle) {
  { ctx.alloc() } noexcept -> ref_to_stack_allocator;
  { ctx.push(handle) } -> std::same_as<void>;
  { ctx.pop() } noexcept -> std::same_as<frame_handle<T>>;
};

/**
 * @brief Fetch the allocator type of a worker context `T`.
 */
template <worker_context T>
using allocator_t = std::remove_reference_t<decltype(std::declval<T &>().alloc())>;

// ==== Forward-decl

export template <returnable T, worker_context Context>
struct task;

// ========== Invocability ========== //

/**
 * @brief Test if a callable `Fn` when invoked with `Args...` returns an `lf::task`.
 */
export template <typename Fn, typename... Args>
concept async_invocable =
    std::invocable<Fn, Args...> && specialization_of<std::invoke_result_t<Fn, Args...>, task>;

/**
 * @brief The result type of invoking an async function `Fn` with `Args...`.
 */
export template <typename Fn, typename... Args>
  requires async_invocable<Fn, Args...>
using async_result_t = std::invoke_result_t<Fn, Args...>::value_type;

/**
 * @brief Subsumes `async_invocable` and checks the result type is `R`.
 */
export template <typename Fn, typename R, typename... Args>
concept async_invocable_to = async_invocable<Fn, Args...> && std::same_as<R, async_result_t<Fn, Args...>>;

} // namespace lf
