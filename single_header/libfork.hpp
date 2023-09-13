
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
#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

#include <memory>
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
 * @brief Shorthand for `std::numeric_limits<std::unt16_t>::max()`.
 */
static constexpr std::uint16_t k_u16_max = std::numeric_limits<std::uint16_t>::max();

/**
 * @brief Shorthand for `std::numeric_limits<std::int16_t>::max()`.
 */
static constexpr std::int16_t k_i16_max = std::numeric_limits<std::int16_t>::max();

/**
 * @brief Shorthand for `std::numeric_limits<std::unt16_t>::min()`.
 */
static constexpr std::int16_t k_i16_min = std::numeric_limits<std::int16_t>::min();

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
#ifndef FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0
#define FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

#ifndef EE6A2701_7559_44C9_B708_474B1AE823B2
#define EE6A2701_7559_44C9_B708_474B1AE823B2

#include <concepts>
#include <semaphore>
#include <type_traits>
#include <utility>


#include "tuplet/tuple.hpp"

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
concept assignable = std::is_lvalue_reference_v<LHS> && requires(LHS lhs, RHS &&rhs) {
  { lhs = std::forward<RHS>(rhs) } -> std::same_as<LHS>;
};

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
template <typename T>
struct promise_result<void, T> {
  constexpr void return_void() const noexcept {}
};

template <>
struct promise_result<root_result<void>, void> {

  constexpr void return_void() const noexcept {}

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
    tuplet::apply(emplace, std::move(args));
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
      (*ret) = R(std::forward<Args>(args)...);
    } else {
      (*ret) = T(std::forward<Args>(args)...);
    }
  };

  R *m_ret_address;
};

// ----------------------------------------------------- //

} // namespace lf

#endif /* EE6A2701_7559_44C9_B708_474B1AE823B2 */
#ifndef D66428B1_3B80_45ED_A7C2_6368A0903810
#define D66428B1_3B80_45ED_A7C2_6368A0903810

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



/**
 * @file stack.hpp
 *
 * @brief Provides an async cactus-stack, control blocks and memory management.
 */

namespace lf {

// -------------------- Forward decls -------------------- //

class async_stack;

class frame_block;

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

/**
 * @brief The async stack pointer.
 *
 * \rst
 *
 * Each thread has a pointer into its current async stack, the end of the stack is marked with a sentinel
 * `frame_block`. It is assumed that the runtime initializes this to a sentinel-guarded `async_stack`. A
 * stack looks a little bit like this:
 *
 * .. code::
 *
 *    |--------------- async_stack object ----------------|
 *
 *                 R[coro_frame].....R[coro_frame].......SP
 *                 |---------------->|------------------> |--> Pointer to parent (some other stack)
 *
 * Key:
 * - `R` = regular `frame_block`.
 * - `S` = sentinel `frame_block`.
 * - `.` = padding.
 *
 * \endrst
 */
constinit inline thread_local frame_block *asp = nullptr;

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
class frame_block : detail::immovable<frame_block>, public debug_block {
public:
  /**
   * @brief For root blocks.
   */
  explicit constexpr frame_block(stdx::coroutine_handle<> coro) noexcept : m_prev{0}, m_coro(offset(coro.address())) {}

  /**
   * @brief For regular blocks -- need to call `set_coro` after construction.
   */
  explicit constexpr frame_block(frame_block *parent) : m_prev{offset(parent)}, m_coro{uninitialized} {}

  /**
   * @brief For sentinel blocks.
   *
   * The `tag` parameter is unused, but is required to disambiguate the constructor without introducing a
   * default constructor that maybe called by accident.
   */
  explicit constexpr frame_block([[maybe_unused]] detail::empty tag) noexcept : m_prev{0}, m_coro{0} {};

  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks
   * and their `tls::asp` will be pointing at a sentinel.
   */
  void resume_stolen() noexcept;

  /**
   * @brief Resume an external task.
   *
   * When this function returns this worker will have run out of tasks
   * and their asp will be pointing at a sentinel.
   */
  template <thread_context Context>
  void resume_external() noexcept;

  /**
   * @brief Set the offset to coro.
   *
   * Only regular blocks should call this as rooted blocks can set it at construction.
   */
  constexpr void set_coro(stdx::coroutine_handle<> coro) noexcept {
    LF_ASSERT(m_coro == uninitialized && m_prev != 0 && m_prev != uninitialized);
    m_coro = offset(coro.address());
  }

  /**
   * @brief Get a handle to the adjacent/paired coroutine frame.
   */
  [[nodiscard]] auto get_coro() const noexcept -> stdx::coroutine_handle<> {
    LF_ASSERT(!is_sentinel());
    return stdx::coroutine_handle<>::from_address(from_offset(m_coro));
  }

  /**
   * @brief The result of a `frame_block::destroy` call, suitable for structured binding.
   */
  struct parent_t {
    frame_block *parent; ///< The parent of the destroyed coroutine on top of the stack.
    bool parent_on_asp;  ///< `true` if the parent is on the same stack as task that was just popped.
  };

  /**
   * @brief Destroy the coroutine on the top of this threads async stack, sets `tls::asp`.
   */
  static auto pop_asp() -> parent_t {

    frame_block *top = tls::asp;
    LF_ASSERT(top);

    // Destroy the coroutine (this does not effect top)
    LF_ASSERT(top->is_regular());
    top->get_coro().destroy();

    tls::asp = std::bit_cast<frame_block *>(top->from_offset(top->m_prev));

    LF_ASSERT(is_aligned(tls::asp, alignof(frame_block)));

    if (tls::asp->is_sentinel()) [[unlikely]] {
      return {tls::asp->read_sentinel_parent(), false};
    }
    return {tls::asp, true};
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] constexpr auto load_joins(std::memory_order order) const noexcept -> std::uint16_t {
    return m_join.load(order);
  }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  constexpr auto fetch_sub_joins(std::uint16_t val, std::memory_order order) noexcept -> std::uint16_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] constexpr auto steals() const noexcept -> std::uint16_t { return m_steal; }

  /**
   * @brief Check if a non-sentinel frame is a root frame.
   */
  [[nodiscard]] constexpr auto is_root() const noexcept -> bool {
    LF_ASSERT(!is_sentinel());
    return !is_regular();
  }

  /**
   * @brief Check if any frame is a sentinel frame.
   */
  constexpr auto is_sentinel() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_coro == 0;
  }

  /**
   * @brief Reset the join and steal counters, must be outside a fork-join region.
   */
  void reset() noexcept {
    LF_ASSERT(!is_sentinel());
    LF_ASSERT(m_steal != 0); // Reset not needed if steal is zero.

    m_steal = 0;

    static_assert(std::is_trivially_destructible_v<decltype(m_join)>);
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    std::construct_at(&m_join, detail::k_u16_max);
  }

private:
  static constexpr std::int16_t uninitialized = 1;

  /**
   * m_prev and m_coro are both offsets to external values hence values 0..sizeof(frame_block) are invalid.
   * Hence, we use 1 to mark uninitialized values.
   *
   * Rooted has:   m_prev == 0 and m_coro != 0
   * Sentinel has: m_prev == 0 and m_coro == 0,
   * Regular has:  m_prev != 0 and m_coro != 0
   */

  std::atomic_uint16_t m_join = detail::k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;                       ///< Number of steals.
  std::int16_t m_prev;                             ///< Distance to previous frame.
  std::int16_t m_coro;                             ///< Offset from `this` to paired coroutine's void handle.

  constexpr auto is_initialised() const noexcept -> bool {
    static_assert(sizeof(frame_block) > uninitialized, "Required for uninitialized to be invalid offset");
    return m_prev != uninitialized && m_coro != uninitialized;
  }

  constexpr auto is_regular() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_prev != 0;
  }

  static constexpr auto is_aligned(void *address, std::size_t align) noexcept -> bool {
    return std::bit_cast<std::uintptr_t>(address) % align == 0;
  }

  /**
   * @brief Compute the offset from `this` to `external`.
   *
   * Satisfies: `external == from_offset(offset(external))`
   */
  [[nodiscard]] constexpr auto offset(void *external) const noexcept -> std::int16_t {
    LF_ASSERT(external);

    std::ptrdiff_t offset = detail::byte_cast(external) - detail::byte_cast(this);

    LF_ASSERT(detail::k_i16_min <= offset && offset <= detail::k_i16_max); // Check fits in i16.
    LF_ASSERT(offset < 0 || sizeof(frame_block) <= offset);                // Check is not internal.

    return static_cast<std::int16_t>(offset);
  }

  /**
   * @brief Compute an external pointer a distance offset from `this`
   *
   * Satisfies: `external == from_offset(offset(external))`
   */
  [[nodiscard]] constexpr auto from_offset(std::int16_t offset) const noexcept -> void * {
    // Check offset is not internal.
    LF_ASSERT(offset < 0 || sizeof(frame_block) <= offset);

    return const_cast<std::byte *>(byte_cast(this) + offset); // Const cast is safe as pointer is external.
  }

  /**
   * @brief Write the parent below a sentinel frame.
   */
  void write_sentinels_parent(frame_block *parent) noexcept {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    new (address) frame_block *{parent};
  }

  /**
   * @brief Read the parent below a sentinel frame.
   */
  auto read_sentinel_parent() noexcept -> frame_block * {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    // Strict aliasing ok as from_offset(...) guarantees external.
    return *std::bit_cast<frame_block **>(address);
  }
};

static_assert(alignof(frame_block) <= detail::k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

// ----------------------------------------------- //

namespace detail {}

/**
 * @brief A fraction of a thread's cactus stack.
 */
class async_stack : detail::immovable<async_stack> {
public:
  /**
   * @brief Construct a new `async_stack`.
   */
  async_stack() noexcept {
    static_assert(std::is_standard_layout_v<async_stack>);
    static_assert(alignof(frame_block) <= detail::k_new_align);
    static_assert(alignof(frame_block) <= alignof(frame_block *));

    static_assert(sizeof(async_stack) == k_size, "Spurious padding in async_stack!");

    // Initialize the sentinel with space for a pointer behind it.
    new (static_cast<void *>(m_buf + k_sentinel_offset)) frame_block{lf::detail::empty{}};
  }

  /**
   * @brief Get a pointer to the sentinel `frame_block` on the stack.
   */
  auto sentinel() noexcept -> frame_block * { return std::launder(std::bit_cast<frame_block *>(m_buf + k_sentinel_offset)); }

  /**
   * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
   */
  static auto unsafe_from_sentinel(frame_block *sentinel) noexcept -> async_stack * {
#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_with_class(&async_stack::m_buf));
#endif
    return std::bit_cast<async_stack *>(std::launder(byte_cast(sentinel) - k_sentinel_offset));
  }

private:
  static constexpr std::size_t k_size = detail::k_kibibyte * LF_ASYNC_STACK_SIZE;
  static constexpr std::size_t k_sentinel_offset = k_size - sizeof(frame_block) - sizeof(frame_block *);

  alignas(detail::k_new_align) std::byte m_buf[k_size]; // NOLINT
};

// ----------------------------------------------- //

namespace tls {

/**
 * @brief Set `tls::asp` to point at `frame`.
 *
 * It must currently be pointing at a sentinel.
 */
template <thread_context Context>
inline void eat(frame_block *frame) {
  LF_LOG("Thread eats a stack");
  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());
  frame_block *prev = std::exchange(tls::asp, frame);
  async_stack *stack = async_stack::unsafe_from_sentinel(prev);
  ctx<Context>->stack_push(stack);
}

} // namespace tls

// ----------------------------------------------- //

/**
 * @brief Resume a stolen task.
 *
 * When this function returns this worker will have run out of tasks
 * and their `tls::asp` will be pointing at a sentinel.
 */
inline void frame_block::resume_stolen() noexcept {
  LF_LOG("Call to resume on stolen task");

  // Link the sentinel to the parent.
  LF_ASSERT(tls::asp);
  tls::asp->write_sentinels_parent(this);

  LF_ASSERT(is_regular()); // Only regular tasks should be stolen.
  m_steal += 1;
  get_coro().resume();

  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());
}

// ----------------------------------------------- //

/**
 * @brief Resume an external task.
 *
 * When this function returns this worker will have run out of tasks
 * and their asp will be pointing at a sentinel.
 */
template <thread_context Context>
inline void frame_block::resume_external() noexcept {

  LF_LOG("Call to resume on external task");

  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());

  LF_ASSERT(!is_sentinel()); // Only regular/root tasks are external

  if (!is_root()) {
    tls::eat<Context>(this);
  }

  get_coro().resume();

  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());
}

// ----------------------------------------------- //

class promise_alloc_heap : frame_block {
protected:
  explicit promise_alloc_heap(std::coroutine_handle<> self) noexcept : frame_block{self} { tls::asp = this; }
};

// ----------------------------------------------- //

/**
 * @brief A base class for promises that allocates on an `async_stack`.
 *
 * When a promise deriving from this class is constructed 'tls::asp' will be set and when it is destroyed 'tls::asp'
 * will be returned to the previous frame.
 */
class promise_alloc_stack {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
    auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
    LF_ASSERT(std::has_single_bit(align));
    LF_ASSERT(align > 0);
    return std::max(align, detail::k_new_align);
  }

protected:
  explicit promise_alloc_stack(std::coroutine_handle<> self) noexcept {
    LF_ASSERT(tls::asp);
    tls::asp->set_coro(self);
  }

public:
  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    return promise_alloc_stack::operator new(size, std::align_val_t{detail::k_new_align});
  }

  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size, std::align_val_t al) noexcept -> void * {

    std::uintptr_t align = unwrap(al);

    frame_block *prev_asp = tls::asp;

    LF_ASSERT(prev_asp);

    auto prev_stack_addr = std::bit_cast<std::uintptr_t>(prev_asp);

    std::uintptr_t coro_frame_addr = (prev_stack_addr - size) & ~(align - 1);

    LF_ASSERT(coro_frame_addr % align == 0);

    std::uintptr_t frame_addr = coro_frame_addr - sizeof(frame_block);

    LF_ASSERT(frame_addr % alignof(frame_block) == 0);

    // Starts the lifetime of the new frame block.
    tls::asp = new (std::bit_cast<void *>(frame_addr)) frame_block{prev_asp};

    return std::bit_cast<void *>(coro_frame_addr);
  }

  /**
   * @brief A noop -- use the destroy method!
   */
  static void operator delete(void *) noexcept {}

  /**
   * @brief A noop -- use the destroy method!
   */
  static void operator delete(void *, std::size_t, std::align_val_t) noexcept {}
};

template <thread_context Context>
void worker_init(Context *context) {
  LF_ASSERT(context);
  LF_ASSERT(!tls::ctx<Context>);
  LF_ASSERT(!tls::asp);

  tls::ctx<Context> = context;
  tls::asp = context->stack_pop()->sentinel();
}

template <thread_context Context>
void worker_finalize(Context *context) {
  LF_ASSERT(context == tls::ctx<Context>);
  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());

  context->stack_push(async_stack::unsafe_from_sentinel(tls::asp));

  tls::asp = nullptr;
  tls::ctx<Context> = nullptr;
}

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <memory>
#include <source_location>
#include <type_traits>
#include <utility>

#include "tuplet/tuple.hpp"


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

namespace detail {

struct task_construct_key {};

} // namespace detail

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T, fixed_string Name = "">
struct task {
  using value_type = T; ///< The type of the value returned by the coroutine.

  explicit(false) constexpr task([[maybe_unused]] detail::task_construct_key key) noexcept {};
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
  requires requires { typename std::remove_cvref_t<T>::context_type; }
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
 * @brief A helper to statically attach a new `context_type` to a `first_arg`.
 */
template <thread_context Context, first_arg T>
struct patched : T {
  using context_type = Context;
};

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
class [[nodiscard("packets must be co_awaited")]] packet : detail::move_only<packet<Head, Tail...>> {
public:
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
  void invoke() && { std::move(m_args).apply(function_of<Head>{}); }

  /**
   * @brief Patch the `Head` type with `Context`
   */
  template <thread_context Context>
  constexpr auto patch_with() && noexcept -> packet<patched<Context, Head>, Tail...> {
    return std::move(m_args).apply([](Head head, Tail &&...tail) {
      return packet<patched<Context, Head>, Tail...>{std::move(head), std::forward<Tail>(tail)...};
    });
  }

private:
  [[no_unique_address]] tuplet::tuple<Head, Tail &&...> m_args;
};

// /**
//  * @brief Deduction guide that forwards its arguments as references.
//  */
// template <typename Head, typename... Tail>
// packet(Head, Tail &&...) -> packet<Head, Tail &&...>;

// ----------------------------------------------- //

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
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg : basic_first_arg<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  explicit constexpr basic_first_arg(return_type &ret) : m_ret{std::addressof(ret)} {}

  [[nodiscard]] constexpr auto address() const noexcept -> return_type * { return m_ret; }

private:
  R *m_ret;
};

static_assert(first_arg<basic_first_arg<void, tag::root, decltype([] {})>>);
static_assert(first_arg<basic_first_arg<int, tag::root, decltype([] {})>>);

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */


/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::detail {

// -------------------------------------------------------------------------- //

// TODO: Cleanup below

// /**
//  * @brief Disable rvalue references for T&& template types if an async function
//  * is forked.
//  *
//  * This is to prevent the user from accidentally passing a temporary object to
//  * an async function that will then destructed in the parent task before the
//  * child task returns.
//  */
// template <typename T, typename Self>
// concept protect_forwarding_tparam = first_arg<Self> && !std::is_rvalue_reference_v<T> &&
//                                     (tag_of<Self> != tag::fork || std::is_reference_v<T>);

/**
 * @brief An awaitable type (in a task) that triggers a join.
 */
struct join_t {};

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
      : allocator<Tag>{handle_t::from_promise(*this)},
        promise_result<R, T>{head.address()} {}

  constexpr promise_type() noexcept : allocator<Tag>{handle_t::from_promise(*this)} {}

  static auto get_return_object() noexcept -> task_construct_key { return {}; }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  void unhandled_exception() noexcept { LF_RETHROW; }

  auto final_suspend() noexcept {
    struct final_awaitable : stdx::suspend_always {
      constexpr auto await_suspend(stdx::coroutine_handle<promise_type> child) const noexcept -> stdx::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LF_LOG("Root task at final suspend, releases semaphore");
          // Finishing a root task implies our stack is empty and should have no exceptions.
          child.promise().address()->semaphore.release();
          child.destroy();
          return stdx::noop_coroutine();
        }

        // Completing a non-root task means we currently own the async_stack this child is on

        FATAL_IN_DEBUG(tls::asp->debug_count() == 0, "Fork/Call without a join!");

        LF_ASSERT(tls::asp->steals() == 0);                                      // Fork without join.
        LF_ASSERT(tls::asp->load_joins(std::memory_order_acquire) == k_u16_max); // Destroyed in invalid state.

        LF_LOG("Task reaches final suspend");

        auto [parent, parent_on_asp] = frame_block::pop_asp();

        if constexpr (Tag == tag::call || Tag == tag::invoke) {
          LF_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent->get_coro();
        }

        Context *context = tls::ctx<Context>;

        LF_ASSERT(context);

        if (frame_block *parent_task = context->task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LF_LOG("Parent not stolen, keeps ripping");
          LF_ASSERT(parent_task == parent);
          LF_ASSERT(parent_on_asp);
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occurred so we do not need to call reset().;
          return parent->get_coro();
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

          if (!parent_on_asp) {
            if (!parent->is_root()) [[likely]] {
              tls::eat<Context>(parent);
            }
          }

          // Must reset parents control block before resuming parent.
          parent->reset();

          return parent->get_coro();
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LF_LOG("Task is not last to join");

        if (parent_on_asp) {
          // We are unable to resume the parent, as the resuming thread will take
          // ownership of the parent's stack we must give it up.
          LF_LOG("Thread releases control of parent's stack");
          tls::asp = context->stack_pop()->sentinel();
        }

        LF_ASSERT(tls::asp->is_sentinel());

        return stdx::noop_coroutine();
      }
    };

    return final_awaitable{};
  }

  // public:
  //   template <typename R, typename F, typename... This, typename... Args>
  //   [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<R, tag::fork, F, This...>, Args...> &&packet) {

  //     this->debug_inc();

  //     auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

  //     stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

  //     struct awaitable : stdx::suspend_always {
  //       [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> parent) noexcept ->
  //       decltype(child) {
  //         // In case *this (awaitable) is destructed by stealer after push
  //         stdx::coroutine_handle stack_child = m_child;

  //         LF_LOG("Forking, push parent to context");

  //         Context::context().task_push(task_handle{promise_type::cast_down(parent)});

  //         return stack_child;
  //       }

  //       decltype(child) m_child;
  //     };

  //     return awaitable{{}, child};
  //   }

  //   template <typename R, typename F, typename... This, typename... Args>
  //   [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<R, tag::call, F, This...>, Args...> &&packet) {

  //     this->debug_inc();

  //     auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

  //     stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

  //     struct awaitable : stdx::suspend_always {
  //       [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept
  //           -> decltype(child) {
  //         return m_child;
  //       }
  //       decltype(child) m_child;
  //     };

  //     return awaitable{{}, child};
  //   }

  //   /**
  //    * @brief An invoke should never occur within an async scope as the exceptions will get muddled
  //    */
  // template <typename F, typename... This, typename... Args>
  // [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet) {

  //   FATAL_IN_DEBUG(this->debug_count() == 0, "Invoke within async scope!");

  //   using value_type_child = typename packet<first_arg_t<void, tag::invoke, F, This...>, Args...>::value_type;

  //   using wrapped_value_type =
  //       std::conditional_t<std::is_reference_v<value_type_child>,
  //                          std::reference_wrapper<std::remove_reference_t<value_type_child>>, value_type_child>;

  //   using return_type =
  //       std::conditional_t<std::is_void_v<value_type_child>, regular_void, std::optional<wrapped_value_type>>;

  //   using packet_type = packet<shim_with_context<return_type, Context, first_arg_t<void, tag::invoke, F, This...>>,
  //   Args...>;

  //   using handle_type = typename packet_type::handle_type;

  //   static_assert(std::same_as<value_type_child, typename packet_type::value_type>,
  //                 "An async function's value_type must be return_address_t independent!");

  //   struct awaitable : stdx::suspend_always {

  //     explicit constexpr awaitable(promise_type *in_self,
  //                                  packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet)
  //         : self(in_self),
  //           m_child(packet_type{m_res, {std::move(in_packet.context)}, std::move(in_packet.args)}.invoke_bind(
  //               cast_down(stdx::coroutine_handle<promise_type>::from_promise(*self)))) {}

  //     [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept
  //         -> handle_type {
  //       return m_child;
  //     }

  //     [[nodiscard]] constexpr auto await_resume() -> value_type_child {

  //       LF_ASSERT(self->steals() == 0);

  //       // Propagate exceptions.
  //       if constexpr (LF_PROPAGATE_EXCEPTIONS) {
  //         if constexpr (Tag == tag::root) {
  //           self->get_return_address_obj().exception.rethrow_if_unhandled();
  //         } else {
  //           virtual_stack::from_address(self)->rethrow_if_unhandled();
  //         }
  //       }

  //       if constexpr (!std::is_void_v<value_type_child>) {
  //         LF_ASSERT(m_res.has_value());
  //         return std::move(*m_res);
  //       }
  //     }

  //     return_type m_res;
  //     promise_type *self;
  //     handle_type m_child;
  //   };

  //   return awaitable{this, std::move(in_packet)};
  // }

  // constexpr auto await_transform([[maybe_unused]] join_t join_tag) noexcept {
  //   struct awaitable {
  //   private:
  //     constexpr void take_stack_reset_control() const noexcept {
  //       // Steals have happened so we cannot currently own this tasks stack.
  //       LF_ASSERT(self->steals() != 0);

  //       if constexpr (Tag != tag::root) {

  //         LF_LOG("Thread takes control of task's stack");

  //         Context &context = Context::context();

  //         auto tasks_stack = virtual_stack::from_address(self);
  //         auto thread_stack = context.stack_top();

  //         LF_ASSERT(thread_stack != tasks_stack);
  //         LF_ASSERT(thread_stack->empty());

  //         context.stack_push(tasks_stack);
  //       }

  //       // Some steals have happened, need to reset the control block.
  //       self->reset();
  //     }

  //   public:
  //     [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
  //       // If no steals then we are the only owner of the parent and we are ready to join.
  //       if (self->steals() == 0) {
  //         LF_LOG("Sync ready (no steals)");
  //         // Therefore no need to reset the control block.
  //         return true;
  //       }
  //       // Currently:            joins() = k_imax - num_joined
  //       // Hence:       k_imax - joins() = num_joined

  //       // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
  //       // better if we see all the decrements to joins() and avoid suspending
  //       // the coroutine if possible. Cannot fetch_sub() here and write to frame
  //       // as coroutine must be suspended first.
  //       auto joined = k_imax - self->joins().load(std::memory_order_acquire);

  //       if (self->steals() == joined) {
  //         LF_LOG("Sync is ready");

  //         take_stack_reset_control();

  //         return true;
  //       }

  //       LF_LOG("Sync not ready");
  //       return false;
  //     }

  //     [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> task) noexcept
  //         -> stdx::coroutine_handle<> {
  //       // Currently        joins  = k_imax - num_joined
  //       // We set           joins  = joins() - (k_imax - num_steals)
  //       //                         = num_steals - num_joined

  //       // Hence            joined = k_imax - num_joined
  //       //         k_imax - joined = num_joined

  //       auto steals = self->steals();
  //       auto joined = self->joins().fetch_sub(k_imax - steals, std::memory_order_release);

  //       if (steals == k_imax - joined) {
  //         // We set joins after all children had completed therefore we can resume the task.

  //         // Need to acquire to ensure we see all writes by other threads to the result.
  //         std::atomic_thread_fence(std::memory_order_acquire);

  //         LF_LOG("Wins join race");

  //         take_stack_reset_control();

  //         return task;
  //       }
  //       // Someone else is responsible for running this task and we have run out of work.
  //       LF_LOG("Looses join race");

  //       // We cannot currently own this stack.

  //       if constexpr (Tag != tag::root) {
  //         LF_ASSERT(virtual_stack::from_address(self) != Context::context().stack_top());
  //       }
  //       LF_ASSERT(Context::context().stack_top()->empty());

  //       return stdx::noop_coroutine();
  //     }

  //     constexpr void await_resume() const {
  //       LF_LOG("join resumes");
  //       // Check we have been reset.
  //       LF_ASSERT(self->steals() == 0);
  //       LF_ASSERT(self->joins() == k_imax);

  //       self->debug_reset();

  //       if constexpr (Tag != tag::root) {
  //         LF_ASSERT(virtual_stack::from_address(self) == Context::context().stack_top());
  //       }

  //       // Propagate exceptions.
  //       if constexpr (LF_PROPAGATE_EXCEPTIONS) {
  //         if constexpr (Tag == tag::root) {
  //           self->get_return_address_obj().exception.rethrow_if_unhandled();
  //         } else {
  //           virtual_stack::from_address(self)->rethrow_if_unhandled();
  //         }
  //       }
  //     }

  //     promise_type *self;
  //   };

  //   return awaitable{this};
  // }
};

#undef FATAL_IN_DEBUG

} // namespace lf::detail

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::is_task Task, lf::first_arg Head, typename... Args>
struct lf::stdx::coroutine_traits<Task, Head, Args...> {
  using promise_type =
      lf::detail::promise_type<lf::return_of<Head>, lf::value_of<Task>, lf::context_of<Head>, lf::tag_of<Head>>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::is_task Task, lf::not_first_arg This, lf::first_arg Head, typename... Args>
struct lf::stdx::coroutine_traits<Task, This, Head, Args...> : lf::stdx::coroutine_traits<Task, Head, Args...> {};

#endif /* LF_DOXYGEN_SHOULD_SKIP_THIS */

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */


/**
 * @file core.hpp
 *
 * @brief Meta header which includes all of ``libfork/core/*.hpp``.
 */

#endif /* A6BE090F_9077_40E8_9B57_9BAFD9620469 */


// #include "libfork/schedule/busy.hpp"
// #include "libfork/schedule/inline.hpp"

#endif /* EDCA974A_808F_4B62_95D5_4D84E31B8911 */
