module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:utility;

import std;

namespace lf {

/**
 * @brief Safe integral cast, will terminate if the cast would overflow in debug.
 */
template <std::integral To, std::integral From>
[[nodiscard]]
auto safe_cast(From val) noexcept -> To {

  constexpr auto to_min = std::numeric_limits<To>::min();
  constexpr auto to_max = std::numeric_limits<To>::max();

  constexpr auto from_min = std::numeric_limits<From>::min();
  constexpr auto from_max = std::numeric_limits<From>::max();

  if constexpr (std::cmp_greater(to_min, from_min)) {
    LF_ASSUME(val >= static_cast<From>(to_min));
  }

  if constexpr (std::cmp_less(to_max, from_max)) {
    LF_ASSUME(val <= static_cast<From>(to_max));
  }

  return static_cast<To>(val);
}

/**
 * @brief Assume a pointer is not null, otherwise terminates the program in debug mode.
 */
template <typename T>
[[nodiscard]]
constexpr auto
not_null(T *ptr, std::source_location const loc = std::source_location::current()) noexcept -> T * {
  if (!ptr) {
#ifdef NDEBUG
    std::unreachable();
#else
    impl::terminate_with("Null pointer dereferenced!", loc.file_name(), safe_cast<int>(loc.line()));
#endif
  }
  return ptr;
}

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

/**
 * @brief Basic implementation of a Golang-like defer.
 *
 * \rst
 *
 * Use like:
 *
 * .. code::
 *
 *    auto * ptr = c_api_init();
 *
 *    defer _ = [&ptr] () noexcept {
 *      c_api_clean_up(ptr);
 *    };
 *
 *    // Code that may throw
 *
 * \endrst
 */
template <class F>
  requires std::is_nothrow_invocable_v<F>
class [[nodiscard("Defer will execute unless bound to a name!")]] defer : immovable {
 public:
  /**
   * @brief Construct a new Defer object.
   *
   * @param f Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(F &&f) noexcept(std::is_nothrow_constructible_v<F, F &&>) : m_f(std::forward<F>(f)) {}

  /**
   * @brief Calls the invocable.
   */
  LF_FORCE_INLINE constexpr ~defer() noexcept { std::invoke(std::forward<F>(m_f)); }

 private:
  [[no_unique_address]]
  F m_f;
};

} // namespace lf
