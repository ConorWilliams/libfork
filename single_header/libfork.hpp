
//---------------------------------------------------------------//
//        This is a machine generated file DO NOT EDIT IT        //
//---------------------------------------------------------------//

#ifndef EDCA974A_808F_4B62_95D5_4D84E31B8911
#define EDCA974A_808F_4B62_95D5_4D84E31B8911
#ifndef A6BE090F_9077_40E8_9B57_9BAFD9620469
#define A6BE090F_9077_40E8_9B57_9BAFD9620469

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>
#include <utility>
#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>
#include <concepts>
#include <functional>
#include <memory>
#include <source_location>
#include <type_traits>
#include <utility>
#ifndef EE6A2701_7559_44C9_B708_474B1AE823B2
#define EE6A2701_7559_44C9_B708_474B1AE823B2

#include <concepts>
#include <semaphore>
#include <type_traits>
#include <utility>
#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

#include <memory>
#include <type_traits>
#include <utility>
#ifndef C5DCA647_8269_46C2_B76F_5FA68738AEDA
#define C5DCA647_8269_46C2_B76F_5FA68738AEDA

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include <version>

/**
 * @file macro.hpp
 *
 * @brief A collection of internal and public macros.
 */

// NOLINTBEGIN Sometime macros are the only way to do things...

/**
 * @brief The major version of libfork.
 *
 * Changes with incompatible API/ABI changes.
 */
#define LF_VERSION_MAJOR 3
/**
 * @brief The minor version of libfork.
 *
 * Changes when functionality is added in an API/ABI backward compatible manner.
 */
#define LF_VERSION_MINOR 1
/**
 * @brief The patch version of libfork.
 *
 * Changes when bug fixes are made in an API/ABI backward compatible manner.
 */
#define LF_VERSION_PATCH 0

namespace detail {

#define LF_CONCAT_IMPL(x, y) x##y
#define LF_CONCAT(x, y) LF_CONCAT_IMPL(x, y)

} // namespace detail

/**
 * @brief Use with ``inline namespace`` to mangle the major version number into the symbol names.
 *
 */
#define LF_API LF_CONCAT(v, LF_VERSION_MAJOR)

/**
 * @brief Use with ``inline namespace`` to alter the symbols of classes with different ABI in debug/release
 * mode.
 *
 */
#ifdef NDEBUG
  #define LF_DEPENDENT_ABI release_abi
#else
  #define LF_DEPENDENT_ABI debug_abi
#endif

#ifndef LF_ASYNC_STACK_SIZE
  /**
   * @brief A customizable stack size for ``async_stack``'s (in kibibytes).
   *
   * You can override this by defining ``LF_ASYNC_STACK_SIZE`` to whatever you like.
   */
  #define LF_ASYNC_STACK_SIZE 1024
#endif

static_assert(LF_ASYNC_STACK_SIZE >= 1, "LF_ASYNC_STACK_SIZE must be at least 1 kilobyte");

/**
 * @brief Use to decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) with ``static`` if
 * supported.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CALL static
#else
  #define LF_STATIC_CALL
#endif

/**
 * @brief Use with ``LF_STATIC_CALL`` to decorate ``operator()`` with ``const`` if supported.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CONST
#else
  #define LF_STATIC_CONST const
#endif

// clang-format off

/**
 * @brief Use like `BOOST_HOF_RETURNS` to define a function/lambda with all the noexcept/requires/decltype specifiers.
 * 
 */
#define LF_HOF_RETURNS(expr) noexcept(noexcept(expr)) -> decltype(expr) requires requires { expr; } { return expr;}

// clang-format on

/**
 * @brief Lift an overload-set/template into a constrained lambda.
 */
#define LF_LIFT(overload_set)                                                                                               \
  [](auto &&...args) LF_STATIC_CALL LF_HOF_RETURNS(overload_set(std::forward<decltype(args)>(args)...))

/**
 * @brief Detects if the compiler has exceptions enabled.
 *
 * Overridable by defining ``LF_COMPILER_EXCEPTIONS``.
 */
#ifndef LF_COMPILER_EXCEPTIONS
  #if defined(__cpp_exceptions) || (defined(_MSC_VER) && defined(_CPPUNWIND)) || defined(__EXCEPTIONS)
    #define LF_COMPILER_EXCEPTIONS 1
  #else
    #define LF_COMPILER_EXCEPTIONS 0
  #endif
#endif

#if LF_COMPILER_EXCEPTIONS || defined(LF_DOXYGEN_SHOULD_SKIP_THIS)
  /**
   * @brief Expands to ``try`` if exceptions are enabled, otherwise expands to ``if constexpr (true)``.
   */
  #define LF_TRY try
  /**
   * @brief Expands to ``catch (...)`` if exceptions are enabled, otherwise expands to ``if constexpr
   * (false)``.
   */
  #define LF_CATCH_ALL catch (...)
  /**
   * @brief Expands to ``throw X`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_THROW(X) throw X
  /**
   * @brief Expands to ``throw`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_RETHROW throw
#else
  #define LF_TRY if constexpr (true)
  #define LF_CATCH_ALL if constexpr (false)
  #ifndef NDEBUG
    #define LF_THROW(X) assert(false && "Tried to throw: " #X)
  #else
    #define LF_THROW(X) std::terminate()
  #endif
  #ifndef NDEBUG
    #define LF_RETHROW assert(false && "Tried to rethrow without compiler exceptions")
  #else
    #define LF_RETHROW std::terminate()
  #endif
#endif

/**
 * @brief A wrapper for C++23's ``[[assume(expr)]]`` attribute.
 *
 * Reverts to compiler specific implementations if the attribute is not
 * available.
 *
 * \rst
 *
 *  .. warning::
 *
 *    Using some intrinsics (i.e. GCC's ``__builtin_unreachable()``) this has
 *    different semantics than ``[[assume(expr)]]`` as it WILL evaluate the
 *    expression at runtime. Hence you should conservatively only use this macro
 *    if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */
#if __has_cpp_attribute(assume)
  #define LF_ASSUME(expr) [[assume(bool(expr))]]
#elif defined(__clang__)
  #define LF_ASSUME(expr) __builtin_assume(bool(expr))
#elif defined(__GNUC__) && !defined(__ICC)
  #define LF_ASSUME(expr)                                                                                                   \
    if (bool(expr)) {                                                                                                       \
    } else {                                                                                                                \
      __builtin_unreachable();                                                                                              \
    }
#elif defined(_MSC_VER) || defined(__ICC)
  #define LF_ASSUME(expr) __assume(bool(expr))
#else
  #warning "No LF_ASSUME() implementation for this compiler."
  #define LF_ASSUME(expr)                                                                                                   \
    do {                                                                                                                    \
    } while (false)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  ``LF_ASSUME(expr)`` otherwise
 * ``assert(expr)``.
 */
#ifndef NDEBUG
  #define LF_ASSERT(expr) assert(expr)
#else
  #define LF_ASSERT(expr) LF_ASSUME(expr)
#endif

/**
 * @brief A customizable logging macro.
 *
 * By default this is a no-op. Defining ``LF_LOGGING`` will enable a default
 * logging implementation which prints to ``std::cout``. Overridable by defining your
 * own ``LF_LOG`` macro. API like ``std::format()``.
 */
#ifndef LF_LOG
  #ifdef LF_LOGGING
    #include <iostream>
    #include <mutex>
    #include <thread>
    #include <type_traits>

    #ifdef __cpp_lib_format
      #include <format>
      #define LF_FORMAT(message, ...) std::format((message)__VA_OPT__(, ) __VA_ARGS__)
    #else
      #define LF_FORMAT(message, ...) (message)
    #endif

    #ifdef __cpp_lib_syncbuf
      #include <syncstream>
      #define LF_SYNC_COUT std::osyncstream(std::cout) << std::this_thread::get_id()
    #else
      #define LF_SYNC_COUT std::cout << std::this_thread::get_id()
    #endif

    #define LF_LOG(message, ...)                                                                                            \
      do {                                                                                                                  \
        if (!std::is_constant_evaluated()) {                                                                                \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n';                                    \
        }                                                                                                                   \
      } while (false)
  #else
    #define LF_LOG(head, ...)                                                                                               \
      do {                                                                                                                  \
      } while (false)
  #endif
#endif

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */
#ifndef DF63D333_F8C0_4BBA_97E1_32A78466B8B7
#define DF63D333_F8C0_4BBA_97E1_32A78466B8B7

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

/**
 * @file utility.hpp
 *
 * @brief A collection of internal utilities.
 */

namespace lf::detail {

/**
 * @brief The cache line size (bytes) of the current architecture.
 */
#ifdef __cpp_lib_hardware_interference_size
inline constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t k_cache_line = 64;
#endif

/**
 * @brief The default alignment of `operator new`, a power of two.
 */
inline constexpr std::size_t k_new_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

static_assert(std::has_single_bit(k_new_align));

/**
 * @brief Shorthand for `std::numeric_limits<std::unt32_t>::max()`.
 */
static constexpr std::uint32_t k_u32_max = std::numeric_limits<std::uint32_t>::max();

// /**
//  * @brief Shorthand for `std::numeric_limits<std::uint32_t>::max()`.
//  */
// static constexpr std::uint32_t k_u32_max = std::numeric_limits<std::uint32_t>::max();

inline constexpr std::size_t k_kibibyte = 1024 * 1;          // NOLINT
inline constexpr std::size_t k_mebibyte = 1024 * k_kibibyte; //

/**
 * @brief An empty type that is not copiable or movable.
 *
 * The template parameter prevents multiple empty bases.
 */
template <typename CRTP>
struct immovable {
  immovable() = default;
  immovable(const immovable &) = delete;
  immovable(immovable &&) = delete;
  auto operator=(const immovable &) -> immovable & = delete;
  auto operator=(immovable &&) -> immovable & = delete;
  ~immovable() = default;
};

static_assert(std::is_empty_v<immovable<void>>);

/**
 * @brief An empty type that is only movable.
 */
template <typename CRTP>
struct move_only {
  move_only() = default;
  move_only(const move_only &) = delete;
  move_only(move_only &&) noexcept = default;
  auto operator=(const move_only &) -> move_only & = delete;
  auto operator=(move_only &&) noexcept -> move_only & = default;
  ~move_only() = default;
};

static_assert(std::is_empty_v<immovable<void>>);

/**
 * @brief Invoke a callable with the given arguments, unconditionally noexcept.
 */
template <typename... Args, std::invocable<Args...> Fn>
constexpr auto noexcept_invoke(Fn &&fun, Args &&...args) noexcept -> std::invoke_result_t<Fn, Args...> {
  return std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...);
}

/**
 * @brief Ensure a type is not const/volatile or ref qualified.
 */
template <typename T>
concept unqualified = std::same_as<std::remove_cvref_t<T>, T>;

namespace detail {

template <typename From, typename To>
struct forward_cv;

template <unqualified From, typename To>
struct forward_cv<From, To> : std::type_identity<To> {};

template <unqualified From, typename To>
struct forward_cv<From const, To> : std::type_identity<To const> {};

template <unqualified From, typename To>
struct forward_cv<From volatile, To> : std::type_identity<To volatile> {};

template <unqualified From, typename To>
struct forward_cv<From const volatile, To> : std::type_identity<To const volatile> {};

} // namespace detail

template <typename From, typename To>
using forward_cv_t = typename detail::forward_cv<From, To>::type;

/**
 * @brief Cast a pointer to a byte pointer.
 */
template <typename T>
auto byte_cast(T *ptr) -> forward_cv_t<T, std::byte> * {
  return std::bit_cast<forward_cv_t<T, std::byte> *>(ptr);
}

/**
 * @brief An empty type.
 */
struct empty {};

} // namespace lf::detail

/**
 * @brief Forwards to ``std::is_reference_v<T>``.
 */
template <typename T>
concept reference = std::is_reference_v<T>;

#endif /* DF63D333_F8C0_4BBA_97E1_32A78466B8B7 */


namespace lf {

inline namespace LF_DEPENDENT_ABI {

template <typename T>
concept non_void = !std::is_void_v<T>;

template <non_void T>
class eventually;

/**
 * @brief A wrapper to delay construction of an object.
 *
 * It is up to the caller to guarantee that the object is constructed before it is used and that an object is
 * constructed before the lifetime of the eventually ends (regardless of it is used).
 */
template <non_void T>
  requires std::is_reference_v<T>
class eventually<T> {
public:
  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  constexpr auto operator=(T expr) -> eventually & {
    m_value = std::addressof(expr);
    return *this;
  }

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() noexcept -> T {
    if constexpr (std::is_rvalue_reference_v<T>) {
      return std::move(*m_value);
    } else {
      return m_value;
    }
  }

private:
  std::remove_reference_t<T> *m_value;
};

/**
 * @brief A wrapper to delay construction of an object.
 *
 * It is up to the caller to guarantee that the object is constructed before it is used and that an object is
 * constructed before the lifetime of the eventually ends (regardless of it is used).
 */
template <non_void T>
class eventually : detail::immovable<eventually<T>> {
public:
  // clang-format off

  /**
   * @brief Construct an empty eventually.
   */
  constexpr eventually() noexcept requires std::is_trivially_constructible_v<T> = default;

  // clang-format on

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
  [[nodiscard]] constexpr auto operator*() & noexcept -> T & { return m_value; }

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() && noexcept -> T {
#ifndef NDEBUG
    LF_ASSUME(m_constructed);
#endif
    return std::move(m_value);
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


namespace lf {

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

// ------------------------------ //

// /**
//  * @brief A tag type to explicitly ignore the return value of a task.
//  */
// struct ignore_t {};

/**
 * @brief Like `std::assignable_from` but without the common reference type requirement.
 */
template <typename LHS, typename RHS>
concept assignable = std::is_lvalue_reference_v<LHS> && requires(LHS lhs, RHS &&rhs) { lhs = std::forward<RHS>(rhs); };

/**
 * @brief A tuple-like type with forwarding semantics for in place construction.
 */
template <typename... Args>
struct in_place_args : std::tuple<Args...> {};

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
template <typename T>
struct promise_result<void, T> {
  constexpr void return_void() const noexcept { LF_LOG("return void"); }
};

template <>
struct promise_result<root_result<void>, void> {

  constexpr void return_void() const noexcept { LF_LOG("return void"); }

  explicit constexpr promise_result(root_result<void> *return_address) noexcept : m_ret_address(return_address) {
    LF_ASSERT(return_address);
  }

protected:
  constexpr auto address() const noexcept -> root_result<void> * { return m_ret_address; }

private:
  root_result<void> *m_ret_address;
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
    std::apply(emplace, std::move(args));
  }

  explicit constexpr promise_result(R *return_address) noexcept : m_ret_address(return_address) {
    LF_ASSERT(return_address);
  }

protected:
  constexpr auto address() const noexcept -> R * { return m_ret_address; }

private:
  static constexpr auto emplace = []<typename... Args>(R *ret, Args &&...args) LF_STATIC_CALL {
    if constexpr (requires { ret->emplace(std::forward<Args>(args)...); }) {
      (*ret).emplace(std::forward<Args>(args)...);
    } else if constexpr (std::is_move_assignable_v<R> && std::constructible_from<R, Args...>) {
      // TODO: clang is choking on this...?
      LF_THROW(std::runtime_error("not implemented"));
      // (*ret) = R(std::forward<Args>(args)...);
    } else {
      (*ret) = T(std::forward<Args>(args)...);
    }
  };

  R *m_ret_address;
};

// ----------------------------------------------------- //

} // namespace lf

#endif /* EE6A2701_7559_44C9_B708_474B1AE823B2 */
#ifndef C5C3AA77_D533_4A89_8D33_99BD819C1B4C
#define C5C3AA77_D533_4A89_8D33_99BD819C1B4C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>
#ifndef FE9C96B0_5DDD_4438_A3B0_E77BD54F8673
#define FE9C96B0_5DDD_4438_A3B0_E77BD54F8673

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <version>

/**
 * @file coroutine.hpp
 *
 * @brief Includes \<coroutine\> or \<experimental/coroutine\> depending on the compiler.
 */

#ifndef __has_include
  #error "Missing __has_include macro!"
#endif

// NOLINTBEGIN

#if __has_include(<coroutine>) // Check for a standard library
  #include <coroutine>
namespace lf {
namespace stdx = std;
}
#elif __has_include(<experimental/coroutine>) // Check for an experimental version
  #include <experimental/coroutine>
namespace lf {
namespace stdx = std::experimental;
}
#else
  #error "Missing <coroutine> header!"
#endif

// NOLINTEND

#endif /* FE9C96B0_5DDD_4438_A3B0_E77BD54F8673 */



/**
 * @file stack.hpp
 *
 * @brief Provides an async cactus-stack, control blocks and memory management.
 */

namespace lf {

/**
 * @brief A fraction of a thread's cactus stack.
 */
class async_stack : detail::immovable<async_stack> {
public:
  /**
   * @brief Get a pointer to the sentinel `frame_block` on the stack.
   */
  auto as_bytes() noexcept -> std::byte * { return std::launder(m_buf + k_size); }

  /**
   * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
   */
  static auto unsafe_from_bytes(std::byte *bytes) noexcept -> async_stack * {
#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_with_class(&async_stack::m_buf));
#endif
    return std::launder(std::bit_cast<async_stack *>(bytes - k_size));
  }

private:
  static constexpr std::size_t k_size = detail::k_kibibyte * LF_ASYNC_STACK_SIZE;

  alignas(detail::k_new_align) std::byte m_buf[k_size]; // NOLINT
};

static_assert(std::is_standard_layout_v<async_stack>);
static_assert(sizeof(async_stack) == detail::k_kibibyte * LF_ASYNC_STACK_SIZE, "Spurious padding in async_stack!");

// -------------------- Forward decls -------------------- //

struct frame_block;

// ----------------------------------------------- //

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of
 * tasks. The stack of ``lf::async_stack``s is expected to never be empty, it
 * should always be able to return an empty ``lf::async_stack``.
 */
template <typename Context>
concept thread_context = requires(Context ctx, async_stack *stack, frame_block *ext, frame_block *task) {
  { ctx.max_threads() } -> std::same_as<std::size_t>;        // The maximum number of threads.
  { ctx.submit(ext) };                                       // Submit an external task to the context.
  { ctx.task_pop() } -> std::convertible_to<frame_block *>;  // If the stack is empty, return a null pointer.
  { ctx.task_push(task) };                                   // Push a non-null pointer.
  { ctx.stack_pop() } -> std::convertible_to<async_stack *>; // Return a non-null pointer
  { ctx.stack_push(stack) };                                 // Push a non-null pointer
};

// ----------------------------------------------- //

/**
 * @brief This namespace contains `inline thread_local constinit` variables and functions to manipulate them.
 */
namespace tls {

template <thread_context Context>
constinit inline thread_local Context *ctx = nullptr;

constinit inline thread_local std::byte *asp = nullptr;

} // namespace tls

// ----------------------------------------------- //

/**
 * @brief A base class that (compile-time) conditionally adds debugging information.
 */
struct debug_block {
  // Increase the debug counter
  constexpr void debug_inc() noexcept {
#ifndef NDEBUG
    ++m_count;
#endif
  }

  // Fetch the debug count
  [[nodiscard]] constexpr auto debug_count() const noexcept -> std::int32_t {
#ifndef NDEBUG
    return m_count;
#else
    return 0;
#endif
  }

  // Reset the debug counter
  constexpr void debug_reset() noexcept {
#ifndef NDEBUG
    m_count = 0;
#endif
  }

#ifndef NDEBUG
private:
  std::int32_t m_count = 0; ///< Number of forks/calls (debug).
#endif
};

static_assert(std::is_trivially_destructible_v<debug_block>);

// ----------------------------------------------- //

/**
 * @brief A small bookkeeping struct allocated immediately before/after each coroutine frame.
 */
struct frame_block : detail::immovable<frame_block>, debug_block {

  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks.
   */
  void resume_stolen() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(tls::asp);
    m_steal += 1;
    coro().resume();
  }

  /**
   * @brief Resume an external task.
   *
   * When this function returns this worker will have run out of tasks
   * and their asp will be pointing at a sentinel.
   */
  template <thread_context Context>
  inline void resume_external() noexcept;

/**
 * @brief For non-root tasks.
 */
#ifndef LF_COROUTINE_ABI
  constexpr frame_block(std::coroutine_handle<> coro, std::byte *top) : m_coro{coro}, m_top(top) {}
#else
  constexpr frame_block([[maybe_unused]] std::coroutine_handle<>, std::byte *top) : m_top(top) {}
#endif

  auto set_parent(frame_block *parent) noexcept {
    LF_ASSERT(!m_parent);
    m_parent = parent;
  }

  [[nodiscard]] auto top() const noexcept -> std::byte * {
    LF_ASSERT(!is_root());
    return m_top;
  }

  auto parent() const noexcept -> frame_block * {
    LF_ASSERT(m_parent);
    return m_parent;
  }

  auto coro() noexcept -> std::coroutine_handle<> {
#ifndef LF_COROUTINE_ABI
    return m_coro;
#else
    return std::coroutine_handle<>::from_address(byte_cast(this) - LF_COROUTINE_ABI);
#endif
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] constexpr auto load_joins(std::memory_order order) const noexcept -> std::uint32_t {
    return m_join.load(order);
  }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  constexpr auto fetch_sub_joins(std::uint32_t val, std::memory_order order) noexcept -> std::uint32_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] constexpr auto steals() const noexcept -> std::uint32_t { return m_steal; }

  /**
   * @brief Check if a non-sentinel frame is a root frame.
   */
  [[nodiscard]] constexpr auto is_root() const noexcept -> bool { return m_parent == nullptr; }

  /**
   * @brief Reset the join and steal counters, must be outside a fork-join region.
   */
  void reset() noexcept {

    LF_ASSERT(m_steal != 0); // Reset not needed if steal is zero.

    m_steal = 0;

    static_assert(std::is_trivially_destructible_v<decltype(m_join)>);
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    std::construct_at(&m_join, detail::k_u32_max);
  }

private:
#ifndef LF_COROUTINE_ABI
  std::coroutine_handle<> m_coro;
#endif

  std::byte *m_top;                                ///< Needs to be separate in-case allocation elided.
  frame_block *m_parent = nullptr;                 ///< Same ^
  std::atomic_uint32_t m_join = detail::k_u32_max; ///< Number of children joined (with offset).
  std::uint32_t m_steal = 0;                       ///< Number of steals.
};

static_assert(alignof(frame_block) <= detail::k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

// ----------------------------------------------- //

namespace tls {

/**
 * @brief Set `tls::asp` to point at `frame`.
 *
 * It must currently be pointing at a sentinel.
 */
template <thread_context Context>
inline void eat(std::byte *top) {
  LF_LOG("Thread eats a stack");
  LF_ASSERT(tls::asp);
  std::byte *prev = std::exchange(tls::asp, top);
  LF_ASSERT(prev != top);
  async_stack *stack = async_stack::unsafe_from_bytes(prev);
  ctx<Context>->stack_push(stack);
}

} // namespace tls

// ----------------------------------------------- //

template <thread_context Context>
inline void frame_block::resume_external() noexcept {

  LF_LOG("Call to resume on external task");

  LF_ASSERT(tls::asp);

  if (!is_root()) {
    tls::eat<Context>(top());
  } else {
    LF_LOG("External was root");
  }

  coro().resume();

  LF_ASSERT(tls::asp);
}

// ----------------------------------------------- //

struct promise_alloc_heap : frame_block {
protected:
  explicit promise_alloc_heap(std::coroutine_handle<> self) noexcept : frame_block{self, nullptr} {}
};

// ----------------------------------------------- //

/**
 * @brief A base class for promises that allocates on an `async_stack`.
 *
 * When a promise deriving from this class is constructed 'tls::asp' will be set and when it is destroyed 'tls::asp'
 * will be returned to the previous frame.
 */
struct promise_alloc_stack : frame_block {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  // constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
  //   auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
  //   LF_ASSERT(std::has_single_bit(align));
  //   LF_ASSERT(align > 0);
  //   return std::max(align, detail::k_new_align);
  // }

protected:
  explicit promise_alloc_stack(std::coroutine_handle<> self) noexcept : frame_block{self, tls::asp} {}

public:
  /**
   * @brief Allocate the coroutine on the current `async_stack`.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    LF_ASSERT(tls::asp);

    tls::asp -= (size + detail::k_new_align - 1) & ~(detail::k_new_align - 1);

    LF_LOG("Allocating {} bytes on stack at {}", size, (void *)tls::asp);

    return tls::asp;
  }

  /**
   * @brief Deallocate the coroutine on the current `async_stack`.
   */
  static void operator delete(void *ptr, std::size_t size) noexcept {
    LF_ASSERT(ptr == tls::asp);
    tls::asp += (size + detail::k_new_align - 1) & ~(detail::k_new_align - 1);
    LF_LOG("Deallocating {} bytes on stack to {}", size, (void *)tls::asp);
  }
};

template <thread_context Context>
void worker_init(Context *context) {
  LF_ASSERT(context);
  LF_ASSERT(!tls::ctx<Context>);
  LF_ASSERT(!tls::asp);

  tls::ctx<Context> = context;
  tls::asp = context->stack_pop()->as_bytes();
}

template <thread_context Context>
void worker_finalize(Context *context) {
  LF_ASSERT(context == tls::ctx<Context>);
  LF_ASSERT(tls::asp);

  context->stack_push(async_stack::unsafe_from_bytes(tls::asp));

  tls::asp = nullptr;
  tls::ctx<Context> = nullptr;
}

} // namespace lf

#endif /* C5C3AA77_D533_4A89_8D33_99BD819C1B4C */


/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

template <typename Char, std::size_t N>
struct fixed_string {
private:
  using sloc = std::source_location;

public:
  explicit(false) consteval fixed_string(Char const (&str)[N], sloc loc = sloc::current()) noexcept
      : line{loc.line()},
        column{loc.column()} {
    for (std::size_t i = 0; i < N; ++i) {
      function_name[i] = str[i];
    }
  }

  static constexpr std::size_t file_name_max_size = 127;

  std::array<Char, N> function_name;
  // std::array<Char, file_name_max_size + 1> file_name_buf;
  // std::size_t file_name_size;
  std::uint_least32_t line;
  std::uint_least32_t column;
};

// ----------------------------------------------- //

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T = void, fixed_string Name = "">
struct task {
  using value_type = T; ///< The type of the value returned by the coroutine.

  explicit(false) task(frame_block *frame) : m_frame{frame} { LF_ASSERT(frame); }

  [[nodiscard]] constexpr auto frame() const noexcept -> frame_block * { return m_frame; }

private:
  frame_block *m_frame; ///< The frame block for the coroutine.
};

template <typename>
struct is_task_impl : std::false_type {};

template <typename T, auto Name>
struct is_task_impl<task<T, Name>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

// ----------------------------------------------- //

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 */
enum class tag {
  root,   ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call,   ///< Non root task (on a virtual stack) from an ``lf::call``, completes synchronously.
  fork,   ///< Non root task (on a virtual stack) from an ``lf::fork``, completes asynchronously.
  invoke, ///< Equivalent to ``call`` but caches the return (extra move required).
  tail,   ///< Force a tail-call optimization.
};

// ----------------------------------------------- //

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::context_type`.
 */
template <typename T>
  requires requires { requires thread_context<typename std::remove_cvref_t<T>::context_type>; }
using context_of = typename std::remove_cvref_t<T>::context_type;

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::return_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::return_type; }
using return_of = typename std::remove_cvref_t<T>::return_type;

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::function_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::function_type; }
using function_of = typename std::remove_cvref_t<T>::function_type;

/**
 * @brief A helper to fetch `std::remove_cvref_t<T>::tag_value`.
 */
template <typename T>
  requires requires {
    { std::remove_cvref_t<T>::tag_value } -> std::convertible_to<tag>;
  }
inline constexpr tag tag_of = std::remove_cvref_t<T>::tag_value;

/**
 * @brief A helper to fetch `typename std::remove_cvref_t<T>::value_type`.
 */
template <typename T>
  requires requires { typename std::remove_cvref_t<T>::value_type; }
using value_of = typename std::remove_cvref_t<T>::value_type;

// ----------------------------------------------- //

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

// ----------------------------------------------- //

/**
 * @brief Forward decl for concepts.
 */
template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async;

/**
 * @brief The API of the first arg passed to an async function.
 */
template <typename Arg>
concept first_arg = requires(Arg arg) {
  //
  tag_of<Arg>;

  typename context_of<Arg>;
  typename return_of<Arg>;
  typename function_of<Arg>;

  requires std::is_void_v<return_of<Arg>> || requires {
    { arg.address() } -> std::convertible_to<return_of<Arg> *>;
  };

  []<typename F>(async<F>)
    requires std::same_as<F, function_of<Arg>>
  {
    // Check implicitly convertible to async and that deduced template parameter is the correct type.
  }
  (arg);
};

template <typename T>
concept not_first_arg = !first_arg<T>;

// ----------------------------------------------- //

namespace detail {

template <typename Task, typename Head>
concept valid_return = is_task<Task> && requires { typename promise_result<return_of<Head>, value_of<Task>>; };

} // namespace detail

/**
 * @brief Check that the async function encoded in `Head` is invocable with arguments in `Tail`.
 */
template <typename Head, typename... Tail>
concept valid_packet = first_arg<Head> && detail::valid_return<std::invoke_result_t<function_of<Head>, Head, Tail...>, Head>;

/**
 * @brief A base class for building the first argument to asynchronous functions.
 *
 * This derives from `async<F>` to allow to allow for use as a y-combinator.
 *
 * It needs the true context type to be patched to it.
 *
 * This is used by `std::coroutine_traits` to build the promise type.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg;

/**
 * @brief A helper to statically attach a new `context_type` to a `first_arg`.
 */
template <thread_context Context, first_arg Head>
struct patched : Head {
  using context_type = Context;
};

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
class [[nodiscard("packets must be co_awaited")]] packet : detail::move_only<packet<Head, Tail...>> {
public:
  using task_type = std::invoke_result_t<function_of<Head>, Head, Tail...>;
  using value_type = value_of<task_type>;

  /**
   * @brief Build a packet.
   *
   * It is implicitly constructible because we specify the return type for SFINE and we don't want to
   * repeat the type.
   *
   */
  explicit(false) constexpr packet(Head head, Tail &&...tail) noexcept
      : m_args{std::move(head), std::forward<Tail>(tail)...} {}

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke(frame_block *parent) && -> frame_block *requires(tag_of<Head> != tag::root) {
    auto tsk = std::apply(function_of<Head>{}, std::move(m_args));
    tsk.frame()->set_parent(parent);
    return tsk.frame();
  }

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke() && -> frame_block *requires(tag_of<Head> == tag::root) {
    return std::apply(function_of<Head>{}, std::move(m_args)).frame();
  }

  template <typename F>
  constexpr auto apply(F &&func) && -> decltype(auto) {
    return std::apply(std::forward<F>(func), std::move(m_args));
  }

  /**
   * @brief Patch the `Head` type with `Context`
   */
  template <thread_context Context>
  constexpr auto patch_with() && noexcept -> packet<patched<Context, Head>, Tail...> {
    return std::move(*this).apply([](Head head, Tail &&...tail) -> packet<patched<Context, Head>, Tail...> {
      return {{std::move(head)}, std::forward<Tail>(tail)...};
    });
  }

private:
  [[no_unique_address]] std::tuple<Head, Tail &&...> m_args;
};

// /**
//  * @brief Deduction guide that forwards its arguments as references.
//  */
// template <typename Head, typename... Tail>
// packet(Head, Tail &&...) -> packet<Head, Tail &&...>;

// ----------------------------------------------- //

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async {
  /**
   * @brief Use with explicit template-parameter.
   */
  consteval async() = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide
   * generated from this.
   *
   * This is to allow concise definitions from lambdas:
   *
   * .. code::
   *
   *    constexpr async fib = [](auto fib, ...) -> task<int, "fib"> {
   *        // ...
   *    };
   */
  explicit(false) consteval async([[maybe_unused]] Fn invocable_which_returns_a_task) {}

  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   *
   * Note that the return type is tagged void however during the `await_transform` the full type will be
   * captured.
   */
  template <typename... Args>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept
      -> packet<basic_first_arg<void, tag::invoke, Fn>, Args...> {
    return {{}, std::forward<Args>(args)...};
  }
};

// ----------------------------------------------- //

/**
 * @brief A type that satisfies the ``thread_context`` concept.
 *
 * This is used to detect bad coroutine calls early. All its methods are
 * unimplemented as it is only used in unevaluated contexts.
 */
struct dummy_context {
  auto max_threads() -> std::size_t;
  auto submit(frame_block *) -> void;
  auto task_pop() -> frame_block *;
  auto task_push(frame_block *) -> void;
  auto stack_pop() -> async_stack *;
  auto stack_push(async_stack *) -> void;
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

/**
 * @brief Void/ignore specialization.
 */
template <tag Tag, stateless F>
struct basic_first_arg<void, Tag, F> : async<F>, detail::move_only<basic_first_arg<void, Tag, F>> {
  using context_type = dummy_context;   ///< A default context
  using return_type = void;             ///< The type of the return address.
  using function_type = F;              ///< The underlying async
  static constexpr tag tag_value = Tag; ///< The tag value.

  template <typename R>
  auto rebind(R &ret) const noexcept -> basic_first_arg<R, Tag, F> {
    return {ret};
  }
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg : basic_first_arg<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  explicit(false) constexpr basic_first_arg(return_type &ret) : m_ret{std::addressof(ret)} {}

  [[nodiscard]] constexpr auto address() const noexcept -> return_type * { return m_ret; }

private:
  R *m_ret;
};

static_assert(first_arg<basic_first_arg<void, tag::root, decltype([] {})>>);
static_assert(first_arg<basic_first_arg<int, tag::root, decltype([] {})>>);

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */


/**
 * @file call.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` and
 * ``lf::sync_wait`` machinery.
 */

namespace lf {

namespace detail {

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  #define LF_DEPRECATE [[deprecated("Use operator[] instead")]]
#else
  #define LF_DEPRECATE
#endif

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asynchronous function.
 */
template <tag Tag>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
    requires(Tag != tag::tail)
  LF_DEPRECATE [[nodiscard("HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()(R &ret, [[maybe_unused]] async<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<R, Tag, F>, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  LF_DEPRECATE [[nodiscard("HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()([[maybe_unused]] async<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
    requires(Tag != tag::tail)
  [[nodiscard("HOF needs to be called")]] static constexpr auto operator[](R &ret,
                                                                           [[maybe_unused]] async<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<R, Tag, F>, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard("HOF needs to be called")]] static constexpr auto operator[]([[maybe_unused]] async<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
      return {{}, std::forward<Args>(args)...};
    };
  }
#endif
};

#undef LF_DEPRECATE

struct join_type {};

} // namespace detail

/**
 * @brief An awaitable (in a task) that triggers a join.
 */
inline constexpr detail::join_type join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 */
inline constexpr detail::bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr detail::bind_task<tag::call> call = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a tail-call.
 */
inline constexpr detail::bind_task<tag::tail> tail = {};

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
#ifndef FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0
#define FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>



/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::detail {

// -------------------------------------------------------------------------- //

// TODO: Cleanup below

#ifndef NDEBUG
  #define FATAL_IN_DEBUG(expr, message)                                                                                     \
    do {                                                                                                                    \
      if (!(expr)) {                                                                                                        \
        ::lf::detail::noexcept_invoke([] { LF_THROW(std::runtime_error(message)); });                                       \
      }                                                                                                                     \
    } while (false)
#else
  #define FATAL_IN_DEBUG(expr, message)                                                                                     \
    do {                                                                                                                    \
    } while (false)
#endif

template <tag Tag>
using allocator = std::conditional_t<Tag == tag::root, promise_alloc_heap, promise_alloc_stack>;

template <typename R, typename T, thread_context Context, tag Tag>
struct promise_type : allocator<Tag>, promise_result<R, T> {

  static_assert(Tag == tag::fork || Tag == tag::call || Tag == tag::root);
  static_assert(Tag != tag::root || is_root_result_v<R>);

  using handle_t = stdx::coroutine_handle<promise_type>;

  template <first_arg Head, typename... Tail>
  constexpr promise_type(Head const &head, [[maybe_unused]] Tail &&...tail) noexcept
    requires std::constructible_from<promise_result<R, T>, R *>
      : allocator<Tag>{std::coroutine_handle<>{handle_t::from_promise(*this)}},
        promise_result<R, T>{head.address()} {}

  template <not_first_arg Self, first_arg Head, typename... Tail>
  constexpr promise_type([[maybe_unused]] Self const &self, Head const &head, Tail &&...tail) noexcept
    requires std::constructible_from<promise_result<R, T>, R *>
      : promise_type{head, std::forward<Tail>(tail)...} {}

  constexpr promise_type() noexcept : allocator<Tag>(handle_t::from_promise(*this)) {}

  auto get_return_object() noexcept -> frame_block * { return this; }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  void unhandled_exception() noexcept {
    noexcept_invoke([] { LF_RETHROW; });
  }

  ~promise_type() noexcept { LF_LOG("promise destructs"); }

  auto final_suspend() noexcept {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the async_stack this child is on

    FATAL_IN_DEBUG(this->debug_count() == 0, "Fork/Call without a join!");

    LF_ASSERT(this->steals() == 0);                                      // Fork without join.
    LF_ASSERT(this->load_joins(std::memory_order_acquire) == k_u32_max); // Destroyed in invalid state.

    struct final_awaitable : stdx::suspend_always {
      constexpr auto await_suspend(stdx::coroutine_handle<promise_type> child) const noexcept -> stdx::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LF_LOG("Root task at final suspend, releases semaphore");
          // Finishing a root task implies our stack is empty and should have no exceptions.
          child.promise().address()->semaphore.release();
          child.destroy();
          LF_LOG("Root task yields to executor");
          return stdx::noop_coroutine();
        }

        LF_LOG("Task reaches final suspend");

        frame_block *parent = child.promise().parent();

        child.destroy();

        LF_ASSERT(parent);

        if constexpr (Tag == tag::call) {
          LF_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent->coro();
        }

        // std::cout << "context is " << (void *)(tls::ctx<Context>) << std::endl;

        Context *context = tls::ctx<Context>;

        LF_ASSERT(context);

        if (frame_block *parent_task = context->task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LF_LOG("Parent not stolen, keeps ripping");
          LF_ASSERT(parent_task == parent);
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occurred so we do not need to call reset().;
          return parent->coro();
        }

        // We are either: the thread that created the parent or a thread that completed a forked task.

        // Note: emptying stack implies finished a stolen task or finished a task forked from root.

        // Cases:
        // 1. We are fork_from_root_t
        //    - Every task forked from root is the the first task on a stack -> stack is empty now.
        //      Parent (root) is not on a stack so do not need to take/release control
        // 2. We are fork_t
        //    - Stack is empty -> we cannot be the thread that created the parent as it would be on our stack.
        //    - Stack is non-empty -> we must be the creator of the parent

        // If we created the parent then our current stack is non empty (unless the parent is a root task).
        // If we did not create the parent then we just cleared our current stack and it is now empty.

        LF_LOG("Task's parent was stolen");

        // Register with parent we have completed this child task.
        if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent therefore, we must continue parent.

          LF_LOG("Task is last child to join, resumes parent");

          if (!parent->is_root()) [[likely]] {
            if (parent->top() != tls::asp) {
              tls::eat<Context>(parent->top());
            }
          }

          // Must reset parents control block before resuming parent.
          parent->reset();

          return parent->coro();
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LF_LOG("Task is not last to join");

        if (parent->top() == tls::asp) {
          // We are unable to resume the parent, as the resuming thread will take
          // ownership of the parent's stack we must give it up.
          LF_LOG("Thread releases control of parent's stack");
          tls::asp = context->stack_pop()->as_bytes();
        }

        return stdx::noop_coroutine();
      }
    };

    return final_awaitable{};
  }

  template <first_arg Head, typename... Args>
    requires(tag_of<Head> == tag::fork)
  [[nodiscard]] constexpr auto await_transform(packet<Head, Args...> &&packet)
    requires requires { std::move(packet).template patch_with<Context>(); }
  {

    this->debug_inc();

    frame_block *child = std::move(packet).template patch_with<Context>().invoke(this);

    LF_ASSERT(child);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<promise_type> p) noexcept
          -> stdx::coroutine_handle<> {

        LF_LOG("Forking, push parent to context");

        LF_ASSERT(&p.promise() == m_parent);

        // Need it here (on real stack) in case *this is destructed after push.
        stdx::coroutine_handle child = m_child->coro();

        // std::cout << "context is " << (void *)(tls::ctx<Context>) << std::endl;

        tls::ctx<Context>->task_push(m_parent);

        return child;
      }

      frame_block *m_parent;
      frame_block *m_child;
    };

    return awaitable{{}, this, child};
  }

  template <first_arg Head, typename... Args>
    requires(tag_of<Head> == tag::call)
  [[nodiscard]] constexpr auto await_transform(packet<Head, Args...> &&packet)
    requires requires { std::move(packet).template patch_with<Context>(); }
  {

    frame_block *child = std::move(packet).template patch_with<Context>().invoke(this);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        LF_LOG("Calling");
        return m_child->coro();
      }

      frame_block *m_child;
    };

    return awaitable{{}, child};
  }

  template <typename F, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<basic_first_arg<void, tag::invoke, F>, Args...> &&packet)
    requires std::is_void_v<value_of<lf::packet<basic_first_arg<void, tag::invoke, F>, Args...>>>
  {

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        using new_packet_t = lf::packet<basic_first_arg<void, tag::call, F>, Args...>;

        new_packet_t new_packet = std::move(m_packet).apply([&](auto, Args &&...args) -> new_packet_t {
          return {{}, std::forward<Args>(args)...};
        });

        static_assert(std::is_void_v<value_of<new_packet_t>>, "Value type dependent on first arg!");

        LF_LOG("Invoking");

        return std::move(new_packet).template patch_with<Context>().invoke(parent)->coro();
      }

      frame_block *parent;
      lf::packet<basic_first_arg<void, tag::invoke, F>, Args...> m_packet;
    };

    return awaitable{{}, this, std::move(packet)};
  }

  template <typename F, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<basic_first_arg<void, tag::invoke, F>, Args...> &&packet) {

    using packet_t = lf::packet<basic_first_arg<void, tag::invoke, F>, Args...>;
    using return_t = eventually<value_of<packet_t>>;

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        using new_packet_t = lf::packet<basic_first_arg<return_t, tag::call, F>, Args...>;

        new_packet_t new_packet = std::move(m_packet).apply([&](auto, Args &&...args) -> new_packet_t {
          return {{m_res}, std::forward<Args>(args)...};
        });

        static_assert(std::same_as<value_of<packet_t>, value_of<new_packet_t>>, "Value type dependent on first arg!");

        LF_LOG("Invoking");

        return std::move(new_packet).template patch_with<Context>().invoke(parent)->coro();
      }

      [[nodiscard]] constexpr auto await_resume() -> value_of<packet_t> { return *std::move(m_res); }

      frame_block *parent;
      packet_t m_packet;
      return_t m_res;
    };

    return awaitable{{}, this, std::move(packet), {}};
  }

  constexpr auto await_transform([[maybe_unused]] join_type join_tag) noexcept {
    struct awaitable {
    private:
      constexpr void take_stack_reset_control() const noexcept {
        // Steals have happened so we cannot currently own this tasks stack.
        LF_ASSERT(self->steals() != 0);

        if constexpr (Tag != tag::root) {
          tls::eat<Context>(self->top());
        }
        // Some steals have happened, need to reset the control block.
        self->reset();
      }

    public:
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // If no steals then we are the only owner of the parent and we are ready to join.
        if (self->steals() == 0) {
          LF_LOG("Sync ready (no steals)");
          // Therefore no need to reset the control block.
          return true;
        }
        // Currently:            joins() = k_u32_max - num_joined
        // Hence:       k_u32_max - joins() = num_joined

        // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
        // better if we see all the decrements to joins() and avoid suspending
        // the coroutine if possible. Cannot fetch_sub() here and write to frame
        // as coroutine must be suspended first.
        auto joined = k_u32_max - self->load_joins(std::memory_order_acquire);

        if (self->steals() == joined) {
          LF_LOG("Sync is ready");

          take_stack_reset_control();

          return true;
        }

        LF_LOG("Sync not ready");
        return false;
      }

      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> task) noexcept
          -> stdx::coroutine_handle<> {
        // Currently        joins  = k_u32_max  - num_joined
        // We set           joins  = joins()    - (k_u32_max - num_steals)
        //                         = num_steals - num_joined

        // Hence               joined = k_u32_max - num_joined
        //         k_u32_max - joined = num_joined

        auto steals = self->steals();
        auto joined = self->fetch_sub_joins(k_u32_max - steals, std::memory_order_release);

        if (steals == k_u32_max - joined) {
          // We set joins after all children had completed therefore we can resume the task.

          // Need to acquire to ensure we see all writes by other threads to the result.
          std::atomic_thread_fence(std::memory_order_acquire);

          LF_LOG("Wins join race");

          take_stack_reset_control();

          return task;
        }
        // Someone else is responsible for running this task and we have run out of work.
        LF_LOG("Looses join race");

        // We cannot currently own this stack.

        if constexpr (Tag != tag::root) {
          LF_ASSERT(self->top() != tls::asp);
        }

        return stdx::noop_coroutine();
      }

      constexpr void await_resume() const noexcept {
        LF_LOG("join resumes");
        // Check we have been reset.
        LF_ASSERT(self->steals() == 0);
        LF_ASSERT(self->load_joins(std::memory_order_relaxed) == k_u32_max);

        self->debug_reset();

        if constexpr (Tag != tag::root) {
          LF_ASSERT(self->top() == tls::asp);
        }
      }

      frame_block *self;
    };

    return awaitable{this};
  }
};

#undef FATAL_IN_DEBUG

} // namespace lf::detail

namespace lf {
/**
 * @brief Disable rvalue references for T&& template types if an async function is forked.
 *
 * This is to prevent the user from accidentally passing a temporary object to
 * an async function that will then destructed in the parent task before the
 * child task returns.
 */
template <typename T, tag Tag>

concept no_dangling = Tag != tag::fork || !std::is_rvalue_reference_v<T>;

template <first_arg Head, lf::is_task Task>
using promise_for = detail::promise_type<return_of<Head>, value_of<Task>, context_of<Head>, tag_of<Head>>;

} // namespace lf

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::is_task Task, lf::first_arg Head, lf::no_dangling<lf::tag_of<Head>>... Args>
struct lf::stdx::coroutine_traits<Task, Head, Args...> {
  using promise_type = lf::promise_for<Head, Task>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::is_task Task, lf::not_first_arg This, lf::first_arg Head, lf::no_dangling<lf::tag_of<Head>>... Args>
struct lf::stdx::coroutine_traits<Task, This, Head, Args...> : lf::stdx::coroutine_traits<Task, Head, Args...> {};

#endif /* LF_DOXYGEN_SHOULD_SKIP_THIS */

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
#ifndef E54125F4_034E_45CD_8DF4_7A71275A5308
#define E54125F4_034E_45CD_8DF4_7A71275A5308

#include <type_traits>


namespace lf {

template <typename Sch>
concept scheduler = requires(Sch &&sch, frame_block *ext) {
  typename context_of<Sch>;
  std::forward<Sch>(sch).submit(ext);
};

template <typename Context, typename R, stateless F>
struct root_head : basic_first_arg<R, tag::root, F> {
  using context_type = Context;
};

template <typename Context, stateless F, typename... Args>
struct sync_wait_impl {
  using dummy_packet = packet<root_head<Context, void, F>, Args...>;
  using dummy_packet_value_type = value_of<std::invoke_result_t<F, dummy_packet, Args...>>;

  using real_packet = packet<root_head<Context, root_result<dummy_packet_value_type>, F>, Args...>;
  using real_packet_value_type = value_of<std::invoke_result_t<F, real_packet, Args...>>;

  static_assert(std::same_as<dummy_packet_value_type, real_packet_value_type>, "Value type changes!");
};

template <scheduler Sch, stateless F, typename... Args>
using result_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet_value_type;

template <scheduler Sch, stateless F, typename... Args>
using packet_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet;

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 */
template <scheduler Sch, stateless F, class... Args>
auto sync_wait(Sch &&sch, [[maybe_unused]] async<F> fun, Args &&...args) noexcept -> result_t<Sch, F, Args...> {

  root_result<result_t<Sch, F, Args...>> root_block;

  packet_t<Sch, F, Args...> packet{{{root_block}}, std::forward<Args>(args)...};

  frame_block *ext = std::move(packet).invoke();

  LF_LOG("Submitting root");

  std::forward<Sch>(sch).submit(ext);

  LF_LOG("Aquire semaphore");

  root_block.semaphore.acquire();

  LF_LOG("Semaphore acquired");

  if constexpr (!std::is_void_v<result_t<Sch, F, Args...>>) {
    return *std::move(root_block);
  }
}

} // namespace lf

#endif /* E54125F4_034E_45CD_8DF4_7A71275A5308 */


/**
 * @file core.hpp
 *
 * @brief Meta header which includes all the headers in ``libfork/core``.
 */

#endif /* A6BE090F_9077_40E8_9B57_9BAFD9620469 */


// #include "libfork/schedule/busy.hpp"
// #include "libfork/schedule/inline.hpp"

#endif /* EDCA974A_808F_4B62_95D5_4D84E31B8911 */
