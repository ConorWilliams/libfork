#ifndef EE6A2701_7559_44C9_B708_474B1AE823B2
#define EE6A2701_7559_44C9_B708_474B1AE823B2

#include <concepts>
#include <utility>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "tuplet/tuple.hpp"

namespace lf {

namespace detail {

struct ignore_t {};

} // namespace detail

/**
 * @brief A sentinel value that can be used to explicitly ignore the result of an async function.
 */
inline constexpr auto ignore = detail::ignore_t{};

/**
 * @brief A tuple-like type with forwarding semantics for in place construction.
 */
template <typename... Args>
struct in_place_args : tuplet::tuple<Args...> {};

/**
 * @brief A forwarding deduction guide.
 */
template <typename... Args>
in_place_args(Args &&...) -> in_place_args<Args &&...>;

template <typename R, typename T>
struct promise_result;

/**
 * @brief A promise base-class that provides the return_[...] methods.
 *
 * @tparam R The type of the return address.
 * @tparam T The type of the return value.
 */
template <typename R, typename T>
  requires std::assignable_from<R &, T>
struct promise_result<R, T> {
  /**
   * @brief Assign a value to the return address.
   *
   *  If the return address is directly assignable from `value` this will not construct a temporary.
   */
  constexpr void return_value(T const &value) const
    requires std::constructible_from<T, T const &>
  {
    if constexpr (std::assignable_from<R &, T const &>) {
      *address() = value;
    } else {
      *address() = [&]() -> T { return value; }();
    }
  }

  /**
   * @brief Assign a value directly to the return address.
   */
  constexpr void return_value(T &&value) const
    requires std::constructible_from<T, T>
  {
    *address() = std::move(value);
  }

  /**
   * @brief Assign a value to the return address.
   *
   * If the return address is directly assignable from `value` this will not construct the intermediate `T`.
   */
  template <typename U>
    requires std::constructible_from<T, U>
  constexpr void return_value(U &&value) const {
    if constexpr (std::assignable_from<R &, U>) {
      *address() = std::forward<U>(value);
    } else {
      *address() = [&]() -> T { return std::forward<U>(value); }();
    }
  }

  /**
   * @brief Assign a value constructed from the arguments stored in `args` to the return address.
   *
   * If the return address has a `.emplace()` method that accepts the arguments in the tuple this will be
   * called directly.
   */
  template <reference... Args>
    requires std::constructible_from<T, Args...>
  constexpr void return_value(in_place_args<Args...> args) const {
    tuplet::apply(emplace, std::move(args));
  }

protected:
  explicit constexpr promise_result(non_null<R *> return_address) noexcept : m_ret_address(return_address) {
    LF_ASSERT(return_address);
  }

  constexpr auto address() const noexcept -> non_null<R *> { return m_ret_address; }

private:
  static constexpr auto emplace = [](R *ret, auto &&...args) LF_STATIC_CALL {
    if constexpr (requires { ret->emplace(std::forward<decltype(args)>(args)...); }) {
      (*ret).emplace(std::forward<decltype(args)>(args)...);
    } else {
      (*ret) = [&]() -> T { return {std::forward<decltype(args)>(args)...}; }();
    }
  };

  R *m_ret_address;
};

} // namespace lf

#endif /* EE6A2701_7559_44C9_B708_474B1AE823B2 */
