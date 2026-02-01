module;
export module libfork.core:concepts;

import std;

namespace lf {

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` or a `std::movable` type.
 */
template <typename T>
concept returnable = std::is_void_v<T> || std::movable<T>;

template <typename T>
concept mixinable = std::is_empty_v<T> && !std::is_final_v<T>;

export template <typename T>
concept alloc_mixin = mixinable<T> && requires (std::size_t n, T *ptr) {
  { T::operator new(n) } -> std::same_as<void *>;
  { T::operator delete(ptr, n) } noexcept -> std::same_as<void>;
};

template <typename T, template <typename...> typename Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

/**
 * @brief Test if `T` is a specialization of the template `Template`.
 */
template <typename T, template <typename...> typename Template>
concept specialization_of = is_specialization_of<std::remove_cvref_t<T>, Template>::value;

// Forward-decl
export template <returnable T, alloc_mixin Stack>
class task;

/**
 * @brief Test if a callable `Fn` when invoked with `Args...` returns an `lf::task`.
 */
export template <typename Fn, typename... Args>
concept async_invocable =
    std::invocable<Fn, Args...> && specialization_of<std::invoke_result_t<Fn, Args...>, task>;

/**
 * @brief The result type of invoking an async function `Fn` with `Args...`.
 */
template <typename Fn, typename... Args>
  requires async_invocable<Fn, Args...>
using async_result_t = std::invoke_result_t<Fn, Args...>::value_type;

template <typename Fn, typename R, typename... Args>
concept async_invocable_to = async_invocable<Fn, Args...> && std::same_as<async_result_t<Fn, Args...>, R>;

} // namespace lf
