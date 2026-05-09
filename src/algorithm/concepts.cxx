export module libfork.algorithm:concepts;

import std;

import libfork.core;

namespace lf {

export template <typename T>
concept sized_random_access_range = std::ranges::random_access_range<T> && std::ranges::sized_range<T>;

} // namespace lf
