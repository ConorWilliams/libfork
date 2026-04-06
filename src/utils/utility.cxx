module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.utils:utility;

import std;

namespace lf {

/**
 * @brief Safe integral cast, will terminate if the cast would overflow in debug.
 */
export template <std::integral To, std::integral From>
[[nodiscard]]
constexpr auto safe_cast(From val) noexcept -> To {

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
export template <typename T>
[[nodiscard]]
constexpr auto
not_null(T *ptr, std::source_location const loc = std::source_location::current()) noexcept -> T * {
  if (!ptr) {
#ifdef NDEBUG
    LF_UNREACHABLE();
#else
    impl::terminate_with("Null pointer dereferenced!", loc.file_name(), safe_cast<int>(loc.line()));
#endif
  }
  return ptr;
}

// TODO: get rid of immovable/move_only

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

export class key_t;

export constexpr auto key() noexcept -> key_t;

/**
 * @brief Only way to get one is via un-exported `key()` function.
 */
export class key_t {
 public:
  friend constexpr auto key() noexcept -> key_t { return {}; }

 private:
  constexpr key_t() = default;
};

/**
 * @brief An opaque pointer that only the library can construct.
 */
export class opaque {
 public:
  constexpr opaque() = default;
  constexpr opaque(key_t, void *ptr) noexcept : m_ptr(ptr) {}
  constexpr auto operator==(opaque const &) const noexcept -> bool = default;

  template <typename T>
  [[nodiscard]]
  auto cast(this opaque self) noexcept -> T * {
    return static_cast<T *>(self.m_ptr);
  }

 private:
  void *m_ptr = nullptr;
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
 *    defer _ = [&ptr] noexcept {
 *      c_api_clean_up(ptr);
 *    };
 *
 *    // Code that may throw
 *
 * \endrst
 */
export template <class Fn>
  requires std::is_nothrow_invocable_v<Fn> && std::is_object_v<Fn>
class [[nodiscard("Defer will execute unless bound to a name!")]] defer : immovable {
 public:
  /**
   * @brief Construct a new Defer object.
   *
   * @param fn Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(Fn &&fn) noexcept(std::is_nothrow_constructible_v<Fn, Fn &&>)
      : m_fn(std::forward<Fn>(fn)) {}

  /**
   * @brief Calls the invocable.
   */
  LF_FORCE_INLINE constexpr ~defer() noexcept { std::invoke(std::move(m_fn)); }

 private:
  [[no_unique_address]]
  Fn m_fn;
};

/**
 * @brief Test if a pointer is aligned to a multiple of `Align`.
 *
 * Supports fancy pointers, doesn't require an object to exist at the pointer.
 */
export template <std::size_t Align, typename T>
  requires (std::has_single_bit(Align))
[[nodiscard]]
auto is_sufficiently_aligned(T *ptr) noexcept -> bool {
  return (std::bit_cast<std::uintptr_t>(std::to_address(ptr)) & (Align - 1)) == 0;
}

/**
 * @brief Round up size to a multiple of `Align` for alignment purposes.
 */
export template <std::size_t Align>
  requires (std::has_single_bit(Align))
[[nodiscard]]
constexpr auto round_to_multiple(std::size_t size) noexcept -> std::size_t {
  return (size + Align - 1) & ~(Align - 1);
}

} // namespace lf
