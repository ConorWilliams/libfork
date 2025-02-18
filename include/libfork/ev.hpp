#pragma once

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "libfork/macros/utility.hpp"
#include "libfork/utility.hpp"

namespace lf {

template <typename T>
concept simple =
    std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>;

template <typename T>
class ev : detail::immovable<ev<T>> {
 public:
  template <typename Self>
  constexpr auto operator*(this Self &&self) LF_HOF_RETURNS(*std::forward<Self>(self).m_value)

  template <typename Self>
  constexpr auto operator->(this Self &self) LF_HOF_RETURNS(self.m_value.operator->())

 private:
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr void emplace(Args &&...args) & {
    LF_ASSERT(!m_value.has_value());
    m_value.emplace(std::forward<Args>(args)...);
  }

  std::optional<T> m_value;
};

// Specialization for simple types.

template <simple T>
class ev<T> {
 public:
  template <typename Self>
  constexpr auto operator*(this Self &&self) noexcept -> auto && {
    return std::forward<Self>(self).m_value;
  }

  template <typename Self>
  constexpr auto operator->(this Self &self) LF_HOF_RETURNS(std::addressof(self.m_value))

 private:
  constexpr auto get() & -> T * { return std::addressof(m_value); }

  T m_value;
};

} // namespace lf
