module;
export module libfork.core:concepts;

import std;

namespace lf {

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
concept default_movable = std::movable<T> && std::default_initializable<T>;

// clang-format off

/**
 * @brief Defines the API for a libfork compatible stack allocator.
 *
 * - After construction push is valid.
 * - Pop is valid provided the FILO order is respected.
 * - Destruction is expected to only occur when the stack is empty.
 * - Result of `.checkpoint()` is expected to be "cheap to move".
 * - Release makes the stack empty.
 * - Acquire is only called when the stack is empty
 */
template <typename T>
concept stack_allocator = std::is_object_v<T> && requires (T alloc, std::size_t n, void *ptr) {
    { alloc.empty()                 } noexcept -> std::same_as<bool>;
    { alloc.push(n)                 }          -> std::same_as<void *>;
    { alloc.pop(ptr, n)             } noexcept -> std::same_as<void>;
    { alloc.checkpoint()            } noexcept -> default_movable;
    { alloc.release()               } noexcept -> std::same_as<void>;
    { alloc.acquire(x.checkpoint()) } noexcept -> std::same_as<void>;
    // TODO: test-if-token-on-stack
  };

// clang-format on

template <stack_allocator T>
using checkpoint_t = decltype(std::declval<T &>().checkpoint());

// ==== Context

export template <stack_allocator T>
class frame_handle;

// clang-format off

template <typename T, typename U>
concept context_of = std::is_object_v<T> && stack_allocator<U> && requires (T ctx, frame_handle<U> handle) {
  { ctx.alloc()      } noexcept -> std::same_as<U &>;
  { ctx.push(handle) }          -> std::same_as<void>;
  { ctx.pop()        } noexcept -> std::same_as<frame_handle<U>>;
};

// TODO: shouldn't frame_handle/push/pop be typed on the context?


tepmlate <typename T>
concept lvalue_ref_to_stack_allocator = std::is_lvalue_reference<T> && stack_allocator<std::remove_reference_t<T>>;

// clang-format on

template <typename T>
concept has_allocator = requires (T x) {
  { x.alloc() } noexcept -> lvalue_ref_to_stack_allocator;
};

template <typename T>
  requires std::is_object_v<T> && has_allocator<T>
using allocator_of_t = std::remove_reference_t<decltype(std::declval<T &>().alloc())>;

export template <typename T>
concept context = std::is_object_v<T> && has_allocator<T> && context_of<T, allocator_of_t<T>>;

template <context T>
class arg;

// ==== Forward-decl

export template <returnable T, context Context>
struct task;

template <typename, typename>
struct task_help : std::false_type {};

template <typename T, typename Context>
struct task_help<Context, task<T, Context>> : std::true_type {
  using value_type = T;
};

template <typename Fn, typename Context, typename... Args>
struct task_info : task_help<Context, std::invoke_result_t<Fn, arg<Context>, Args...>> {};

template <typename Fn, typename Context, typename... Args>
concept returns_task = task_info<Fn, Context, Args...>::value;

// ========== Invocability ========== //

/**
 * @brief Test if a callable `Fn` when invoked with `Args...` returns an `lf::task`.
 */
export template <typename Fn, typename Context, typename... Args>
concept async_invocable =
    context<Context> && std::invocable<Fn, arg<Context>, Args...> && returns_task<Fn, Context, Args...>;

/**
 * @brief The result type of invoking an async function `Fn` with `Args...`.
 */
export template <typename Fn, typename Context, typename... Args>
  requires async_invocable<Fn, Context, Args...>
using async_result_t = task_info<Fn, Context, Args...>::value_type;

export template <typename Fn, typename R, typename Context, typename... Args>
concept async_invocable_to =
    async_invocable<Fn, Context, Args...> && std::same_as<R, async_result_t<Fn, Context, Args...>>;

} // namespace lf
