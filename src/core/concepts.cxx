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
concept quasiregular = std::movable<T> && std::default_initializable<T>;

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
concept stack_allocator = quasiregular<T> && requires (T stack, std::size_t n, void *ptr) {
  // Memory
  { stack.push(n) } -> std::same_as<void *>;
  { stack.pop(ptr, n) } noexcept -> std::same_as<void>;
  { stack.empty() } noexcept -> std::same_as<bool>;

  // Checkpointing
  { stack.checkpoint() } noexcept -> std::quasi_regular;
  { stack.release() } noexcept -> std::same_as<void>;
  { stack.acquire(stack.checkpoint()) } noexcept -> std::same_as<void>;
};

template <stack_allocator T>
using checkpoint_t = decltype(std::declval<T>().checkpoint());

// ==== Context

template <typename T, typename Stack>
concept context_of = stack<Stack> == true;

// ==== Forward-decl

// Forward-decl
export template <returnable T, alloc_mixin Stack, typename Context>
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
