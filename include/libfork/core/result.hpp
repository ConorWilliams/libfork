#ifndef EE6A2701_7559_44C9_B708_474B1AE823B2
#define EE6A2701_7559_44C9_B708_474B1AE823B2

#include <concepts>
#include <semaphore>
#include <type_traits>
#include <utility>

#include "libfork/core/eventually.hpp"
#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "tuplet/tuple.hpp"

namespace lf {

/**
 * @brief Like `std::assignable_from` but without the common reference type requirement.
 */
template <typename LHS, typename RHS>
concept assignable = std::is_lvalue_reference_v<LHS> && requires(LHS lhs, RHS &&rhs) {
  { lhs = std::forward<RHS>(rhs) } -> std::same_as<LHS>;
};

// TODO: move wherever invoke goes

struct ignore_t {};

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
 * @brief Specialization for `void` and ignored return types.
 *
 * @tparam R The type of the return address.
 * @tparam T The type of the return value.
 */
template <typename R, typename T>
  requires std::same_as<R, void> or std::same_as<R, ignore_t>
struct promise_result<R, T> {
  constexpr void return_void() const noexcept {}
};

/**
 * @brief A promise base-class that provides the return_[...] methods.
 *
 * @tparam R The type of the return address.
 * @tparam T The type of the return value.
 */
template <typename R, typename T>
  requires assignable<R &, T>
struct promise_result<R, T> {
  /**
   * @brief Assign a value to the return address.
   *
   * If the return address is directly assignable from `value` this will not construct a temporary.
   */
  constexpr void return_value(T const &value) const
    requires std::constructible_from<T, T const &> and (!reference<T>)
  {
    if constexpr (assignable<R &, T const &>) {
      *address() = value;
    } else /* if constexpr (assignable<R &, T>) */ { // ensured by struct constraint
      *address() = T(value);
    }
  }
  /**
   * @brief Assign a value directly to the return address.
   */
  constexpr void return_value(T &&value) const
    requires std::constructible_from<T, T>
  {
    if constexpr (std::is_rvalue_reference_v<T &&>) {
      *address() = std::move(value);
    } else {
      *address() = value;
    }
  }
  /**
   * @brief Assign a value to the return address.
   *
   * If the return address is directly assignable from `value` this will not construct the intermediate `T`.
   */
  template <typename U>
    requires std::constructible_from<T, U>
  constexpr void return_value(U &&value) const {
    if constexpr (assignable<R &, U>) {
      *address() = std::forward<U>(value);
    } else {
      *address() = T(std::forward<U>(value));
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
  static constexpr auto emplace = []<typename... Args>(R *ret, Args &&...args) LF_STATIC_CALL {
    if constexpr (requires { ret->emplace(std::forward<Args>(args)...); }) {
      (*ret).emplace(std::forward<Args>(args)...);
    } else if constexpr (std::is_move_assignable_v<R> && std::constructible_from<R, Args...>) {
      (*ret) = R(std::forward<Args>(args)...);
    } else {
      (*ret) = T(std::forward<Args>(args)...);
    }
  };

  R *m_ret_address;
};

/**
 * @brief A small control structure that a root tasks use to communicate with the main thread.
 */
template <typename T>
struct root_result;

template <>
struct root_result<void> : detail::immovable<root_result<void>> {
  std::binary_semaphore semaphore{0};
};

template <typename T>
struct root_result : eventually<T>, root_result<void> {
  using eventually<T>::operator=;
};

namespace detail {

template <typename T>
struct is_root_result : std::false_type {};

template <typename T>
struct is_root_result<root_result<T>> : std::true_type {};

} // namespace detail

template <typename T>
inline constexpr bool is_root_result_v = detail::is_root_result<T>::value;

// void test() {
//   root_result<int> x;
//   int y;
//   x = y;
// }

} // namespace lf

#endif /* EE6A2701_7559_44C9_B708_474B1AE823B2 */
