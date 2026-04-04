module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:concepts;

import std;

import :utility;

namespace lf {

// =========== Atomic related concepts =========== //

template <typename T>
concept plain_object = std::is_object_v<T> && std::same_as<T, std::remove_reference_t<T>>;

/**
 * @brief Verify a type is suitable for use with `std::atomic`
 *
 * This requires a `TriviallyCopyable` type satisfying both `CopyConstructible` and `CopyAssignable`.
 */
export template <typename T>
concept atomicable = plain_object<T> &&                 //
                     std::is_trivially_copyable_v<T> && //
                     std::is_copy_constructible_v<T> && //
                     std::is_move_constructible_v<T> && //
                     std::is_copy_assignable_v<T> &&    //
                     std::is_move_assignable_v<T>;      //

/**
 * @brief A concept that verifies a type is lock-free when used with `std::atomic`.
 */
export template <typename T>
concept lock_free = atomicable<T> && std::atomic<T>::is_always_lock_free;

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

// ==== Returnable

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` or a `std::movable` type.
 */
export template <typename T>
concept returnable = std::is_void_v<T> || (plain_object<T> && std::movable<T>);

// ==== Allocators

/**
 * @brief Lifted from the c++26 exposition only requirement.
 */
template <class T>
concept simple_allocator =
    std::copy_constructible<T> && std::equality_comparable<T> && requires (T alloc, std::size_t n) {
      { *alloc.allocate(n) } -> std::same_as<typename T::value_type &>;
      { alloc.deallocate(alloc.allocate(n), n) };
    };

/**
 * @brief Semantically `T` must be a std:: Allocator
 *
 * Doesn't specify the full API just 'simple-allocator' exposition only concept.
 */
template <class T, typename U>
concept allocator_of = simple_allocator<T> && std::same_as<typename T::value_type, U>;

// ==== Stack

template <typename T>
  requires std::is_object_v<T>
consteval auto constify(T &&x) noexcept -> std::add_const_t<T> &;

/**
 * @brief Defines the API for a libfork compatible stack.
 *
 * // TODO: define if release is required before acquire?
 *
 * - After construction `this` is empty and push is valid.
 * - Pop is valid provided the FILO order is respected.
 * - Push produces pointers aligned to __STDCPP_DEFAULT_NEW_ALIGNMENT__.
 * - Destruction is expected to only occur when the stack is empty.
 * - Result of `.checkpoint()` is expected to:
 *     - Be "cheap to copy".
 *     - Have a null state (default constructed) that only compares equal to itself.
 *     - Is allowed to return null if push has never been called.
 *     - Compare equal if and only if they belong to the same stack or are both null.
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

/**
 * @brief Fetch the checkpoint type of a stack `T`.
 */
export template <worker_stack T>
using checkpoint_t = decltype(std::declval<T &>().checkpoint());

// ==== Context

export template <typename T>
struct steal_handle;

export template <typename T>
struct sched_handle;

template <typename T>
concept ref_to_worker_stack = std::is_lvalue_reference_v<T> && worker_stack<std::remove_reference_t<T>>;

/**
 * @brief Defines the API for a libfork compatible worker context.
 *
 * This requires that `T` is an object type and supports the following operations:
 *
 * - Push/pop a frame handle onto the context in a LIFO manner.
 * - Have a `worker_stack` that can be accessed via `stack()`.
 * - Post an await handle to the context via `post()` and promise to call resume.
 */
export template <typename T>
concept worker_context =
    plain_object<T> && requires (T context, steal_handle<T> frame, sched_handle<T> await) {
      { context.post(await) } -> std::same_as<void>;
      { context.push(frame) } -> std::same_as<void>;
      { context.pop() } noexcept -> std::same_as<steal_handle<T>>;
      { context.stack() } noexcept -> ref_to_worker_stack;
    };

/**
 * @brief Fetch the stack type of a worker context `T`.
 */
export template <worker_context T>
using stack_t = std::remove_reference_t<decltype(std::declval<T &>().stack())>;

// ==== Forward-decl

export template <worker_context>
struct env {
  explicit constexpr env(key_t) noexcept {}
};

export template <returnable T, worker_context Context>
class task;

// ========== Invocability ========== //

template <typename Context>
struct ctx_invoke_t {
  // Explicitly constrained so overload resolution selects prefers
  template <typename... Args, typename Fn>
    requires std::invocable<Fn, env<Context>, Args...>
  static constexpr auto operator()(Fn &&fn, Args &&...args)
      LF_HOF(std::invoke(std::forward<Fn>(fn), env<Context>{key()}, std::forward<Args>(args)...))

  template <typename... Args, typename Fn>
  static constexpr auto operator()(Fn &&fn, Args &&...args)
      LF_HOF(std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...))
};

template <typename R, typename Context>
concept task_from = specialization_of<R, task> && std::same_as<Context, typename R::context_type>;

/**
 * @brief Test if a callable `Fn` when invoked with `Args...` returns an `lf::task`.
 */
export template <typename Fn, typename Context, typename... Args>
concept async_invocable = worker_context<Context> &&                                                    //
                          std::invocable<ctx_invoke_t<Context>, Fn, Args...> &&                         //
                          task_from<std::invoke_result_t<ctx_invoke_t<Context>, Fn, Args...>, Context>; //

/**
 * @brief Subsumes `async_invocable` and checks that the invocation is `noexcept`.
 */
export template <typename Fn, typename Context, typename... Args>
concept async_nothrow_invocable =
    async_invocable<Fn, Context, Args...> && std::is_nothrow_invocable_v<ctx_invoke_t<Context>, Fn, Args...>;

/**
 * @brief The result type of invoking an async function `Fn` with `Args...`.
 */
export template <typename Fn, typename Context, typename... Args>
  requires async_invocable<Fn, Context, Args...>
using async_result_t = std::invoke_result_t<ctx_invoke_t<Context>, Fn, Args...>::value_type;

/**
 * @brief Subsumes `async_invocable` and checks the result type is `R`.
 */
export template <typename Fn, typename R, typename Context, typename... Args>
concept async_invocable_to =
    async_invocable<Fn, Context, Args...> && std::same_as<R, async_result_t<Fn, Context, Args...>>;

/**
 * @brief Subsumes `async_nothrow_invocable` and `async_invocable_to`.
 */
export template <typename Fn, typename R, typename Context, typename... Args>
concept async_nothrow_invocable_to =
    async_nothrow_invocable<Fn, Context, Args...> && async_invocable_to<Fn, R, Context, Args...>;

} // namespace lf
