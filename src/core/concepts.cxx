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
template <typename T, template <typename...> typename Template>
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
concept default_movable = std::movable<T> && std::default_initializable<T>;

// clang-format off

template <typename Alloc>
concept stack_allocator_help =
  requires (Alloc alloc, std::size_t n, void *ptr) {
    { alloc.empty()                     } noexcept -> std::same_as<bool>;
    { alloc.push(n)                     }          -> std::same_as<void *>;
    { alloc.pop(ptr, n)                 } noexcept -> std::same_as<void>;
    { alloc.checkpoint()                } noexcept -> default_movable;
    { alloc.release()                   } noexcept -> std::same_as<void>;
    { alloc.acquire(alloc.checkpoint()) } noexcept -> std::same_as<void>;
  };

// clang-format on

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
template <typename Alloc>
concept stack_allocator = default_movable<Alloc> && stack_allocator_help<Alloc>;

template <stack_allocator T>
using checkpoint_t = decltype(std::declval<T>().checkpoint());

template <stack_allocator Alloc>
using handle_to_t = frame_handle<checkpoint_t<Alloc>>;

// ==== Context

export template <default_moveable T>
struct frame_type;

struct lock {};

inline constexpr lock key = {};

// TODO: api + test this is lock-free
//
// What is the API:
//  - You can push/pop it
//  - You can convert it to a "steal handle" -> which you can/must resume?
//
// What properties does it have:
//  - It is trivially copyable/constructible/destructible
//  - It has a null value
//  - You can store it in an atomic and it is lock-free
export template <default_movable T>
class frame_handle {
  constexpr frame_handle() = default;
  constexpr frame_handle(std::nullptr_t) noexcept : ptr(nullptr) {}
  constexpr frame_handle(lock, frame_type<T> *ptr) noexcept : ptr(ptr) {}

 private:
  frame_type<T> *ptr;
};

export template <typename T, typename Alloc>
concept context_of = stack_allocator<Alloc> && requires (T *ctx, handle_to_t<Alloc> handle) {
  { ctx->alloc() } noexcept -> std::same_as<Alloc &>;
  { ctx->push(handle) } -> std::same_as<void>;
  { ctx->pop() } noexcept -> std::same_as<handle_to_t<Alloc>>;
};

// ==== Forward-decl

// Forward-decl
export template <returnable T, stack_allocator Alloc, context_of<Alloc> Ctx>
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
using async_result_t = std::invoke_result_t<Fn, Args...>::type;

template <typename Fn, typename R, typename... Args>
concept async_invocable_to = async_invocable<Fn, Args...> && std::same_as<async_result_t<Fn, Args...>, R>;
} // namespace lf
