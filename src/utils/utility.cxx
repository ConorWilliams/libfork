module;
#include "libfork/__impl/assume.hpp"
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
 * @brief Test if a pointer is aligned to a multiple of `Align`.
 *
 * Supports fancy pointers, doesn't require an object to exist at the pointer.
 */
export template <std::size_t Align, typename Ptr>
  requires (std::has_single_bit(Align))
[[nodiscard]]
auto is_sufficiently_aligned(Ptr const &ptr) noexcept -> bool {
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
