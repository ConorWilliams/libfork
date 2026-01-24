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

} // namespace lf
