#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "libfork/macros/assert.hpp"
#include "libfork/macros/utility.hpp"
#include "libfork/utility.hpp"

/**
 * @file ev.hpp
 *
 * @brief The wrapper class for return values from libfork's coroutines.
 */

namespace lf {

/**
 * @brief Test if a type is both trivially default constructible and trivially destructible.
 *
 * @tparam T The type to test.
 */
template <typename T>
concept trivial_return = std::default_initializable<T> && std::is_trivially_destructible_v<T>;

/**
 * @brief A wrapper for return values from libfork's coroutines.
 *
 * Essentially an immovable `std::optional` with a massivly reduced interface.
 * Special handling for `trivial_return` types and reference types.
 *
 * @tparam T The type of the value to wrap.
 */
template <typename T>
class ev : detail::immovable<ev<T>> {
 public:
  template <typename Self>
  [[nodiscard]] constexpr auto operator*(this Self &&self)
      LF_HOF_RETURNS(*std::forward<Self>(self).m_value)

  template <typename Self>
  [[nodiscard]] constexpr auto operator->(this Self &self) LF_HOF_RETURNS(self.m_value.operator->())

  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr void emplace(Args &&...args) & {
    LF_ASSERT(!m_value.has_value());
    m_value.emplace(std::forward<Args>(args)...);
  }

 private:
  std::optional<T> m_value;
};

/**
 * @brief Specialisation of `ev` for `trivial_return` types.
 */
template <trivial_return T>
class ev<T> : detail::immovable<ev<T>> {
 public:
  template <typename Self>
  [[nodiscard]] constexpr auto operator*(this Self &&self) noexcept -> auto && {
    return std::forward<Self>(self).m_value;
  }

  template <typename Self>
  [[nodiscard]] constexpr auto operator->(this Self &self)
      LF_HOF_RETURNS(std::addressof(self.m_value))

  // TODO: support oprator->() for this&& self param

  // [[nodiscard]] constexpr auto operator->() noexcept -> T * { return std::addressof(m_value); }
  // [[nodiscard]] constexpr auto operator->() const noexcept -> T * {
  //   return std::addressof(m_value);
  // }

  constexpr auto get() & -> T * { return std::addressof(m_value); }

 private:
  T m_value;
};

/**
 * @brief A specialisation of `ev` for reference types.
 */
template <typename T>
  requires std::is_reference_v<T>
class ev<T> {
 public:
  template <typename Self>
  [[nodiscard]] constexpr auto operator*(this Self &&self) noexcept -> auto && {
    return std::forward_like<Self>(*self.m_ptr);
  }

  [[nodiscard]] constexpr auto operator->() noexcept -> std::remove_reference_t<T> * {
    return m_ptr;
  }

  [[nodiscard]] constexpr auto operator->() const noexcept -> std::remove_reference_t<T> const * {
    return m_ptr;
  }

 private:
  std::add_pointer_t<std::remove_reference_t<T>> m_ptr;
};

} // namespace lf
