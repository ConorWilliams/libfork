#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

#include <memory>
#include <utility>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

namespace lf {

inline namespace LF_DEPENDENT_ABI {

/**
 * @brief A wrapper to delay construction of an object.
 *
 * It is up to the caller to guarantee that the object is constructed before it is used and that an object is
 * constructed before the lifetime of the eventually ends (regardless of it is used).
 */
template <typename T>
  requires(not std::is_void_v<T>)
class eventually : detail::immovable<eventually<T>> {
public:
  /**
   * @brief Construct an empty eventually.
   */
  constexpr eventually() noexcept
    requires std::is_trivially_constructible_v<T>
  = default;

  constexpr eventually() noexcept : m_init{} {};

  /**
   * @brief Construct an object inside the eventually from ``args...``.
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr void emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    LF_LOG("Constructing an eventually");
#ifndef NDEBUG
    LF_ASSERT(!m_constructed);
#endif
    std::construct_at(std::addressof(m_value), std::forward<Args>(args)...);
#ifndef NDEBUG
    m_constructed = true;
#endif
  }

  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <typename U>
  constexpr auto operator=(U &&expr) noexcept(noexcept(emplace(std::forward<U>(expr)))) -> eventually &
    requires requires { emplace(std::forward<U>(expr)); }
  {
    emplace(std::forward<U>(expr));
    return *this;
  }

  // clang-format off
  constexpr ~eventually() noexcept requires std::is_trivially_destructible_v<T> = default;
  // clang-format on

  constexpr ~eventually() noexcept(std::is_nothrow_destructible_v<T>) {
#ifndef NDEBUG
    LF_ASSUME(m_constructed);
#endif
    std::destroy_at(std::addressof(m_value));
  }

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() & noexcept -> T & {
#ifndef NDEBUG
    LF_ASSUME(m_constructed);
#endif
    return m_value;
  }

private:
  union {
    detail::empty m_init;
    T m_value;
  };

#ifndef NDEBUG
  bool m_constructed = false;
#endif
};

} // namespace LF_DEPENDENT_ABI

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */
