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

} // namespace lf
