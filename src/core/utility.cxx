export module libfork.core:utility;

import std;

namespace lf {

/**
 * @brief An immovable, empty type, use as a mixin.
 */
export struct immovable {
  immovable() = default;
  immovable(const immovable &) = delete;
  immovable(immovable &&) = delete;
  auto operator=(const immovable &) -> immovable & = delete;
  auto operator=(immovable &&) -> immovable & = delete;
  ~immovable() = default;
};

/**
 * @brief A move-only, empty type, use as a mixin.
 */
export struct move_only {
  move_only() = default;
  move_only(const move_only &) = delete;
  move_only(move_only &&) = default;
  auto operator=(const move_only &) -> move_only & = delete;
  auto operator=(move_only &&) -> move_only & = default;
  ~move_only() = default;
};

} // namespace lf
