
//---------------------------------------------------------------//
//        This is a machine generated file DO NOT EDIT IT        //
//---------------------------------------------------------------//

#ifndef EDCA974A_808F_4B62_95D5_4D84E31B8911
#define EDCA974A_808F_4B62_95D5_4D84E31B8911
#ifndef A6BE090F_9077_40E8_9B57_9BAFD9620469
#define A6BE090F_9077_40E8_9B57_9BAFD9620469

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
 * @brief Use to conditionally decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) with ``static``.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CALL static
#else
  #define LF_STATIC_CALL
#endif

/**
 * @brief Use with ``LF_STATIC_CALL`` to conditionally decorate ``operator()`` with ``const``.
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
 * This macro is not truly variadic but the ``...`` allows commas in the macro argument.
 */
#define LF_HOF_RETURNS(...) noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) requires requires { __VA_ARGS__; } { return __VA_ARGS__;}

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
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>


/**
 * @file utility.hpp
 *
 * @brief A collection of internal utilities.
 */

/**
 * @brief The ``libfork`` namespace.
 *
 * Everything in ``libfork`` is contained within this namespace.
 */
namespace lf {

/**
 * @brief An inline namespace that wraps core functionality.
 *
 * This is the namespace that contains the user-facing API of ``libfork``.
 */
inline namespace core {}

/**
 * @brief An inline namespace that wraps extension functionality.
 *
 * This namespace is part of ``libfork``s public API but is intended for advanced users writing schedulers, It exposes the
 * scheduler/context API's alongside some implementation details (such as lock-free queues, and other synchronization
 * primitives) that could be useful when implementing custom schedulers.
 */
inline namespace ext {}

/**
 * @brief An internal namespace that wraps implementation details.
 *
 * \rst
 *
 * .. warning::
 *
 *    This is exposed as internal documentation only it is not part of the public facing API.
 *
 * \endrst
 */
namespace impl {}

} // namespace lf

namespace lf::impl {

// ---------------- Constants ---------------- //

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

/**
 * @brief Number of bytes in a kibibyte.
 */
inline constexpr std::size_t k_kibibyte = 1024;

// ---------------- Utility classes ---------------- //

/**
 * @brief An empty type.
 */
struct empty {};

static_assert(std::is_empty_v<empty>);

// -------------------------------- //

/**
 * @brief An empty base class that is not copiable or movable.
 *
 * The template parameter prevents multiple empty bases when inheriting multiple classes.
 */
template <typename CRTP>
struct immovable {
  immovable(const immovable &) = delete;
  immovable(immovable &&) = delete;
  auto operator=(const immovable &) -> immovable & = delete;
  auto operator=(immovable &&) -> immovable & = delete;

 protected:
  immovable() = default;
  ~immovable() = default;
};

static_assert(std::is_empty_v<immovable<void>>);

// -------------------------------- //

/**
 * @brief An empty base class that is move-only.
 *
 * The template parameter prevents multiple empty bases when inheriting multiple classes.
 */
template <typename CRTP>
struct move_only {

  move_only(const move_only &) = delete;
  move_only(move_only &&) noexcept = default;
  auto operator=(const move_only &) -> move_only & = delete;
  auto operator=(move_only &&) noexcept -> move_only & = default;

  ~move_only() = default;

 protected:
  move_only() = default;
};

static_assert(std::is_empty_v<immovable<void>>);

// ---------------- Meta programming ---------------- //

/**
 * @brief Forwards to ``std::is_reference_v<T>``.
 */
template <typename T>
concept reference = std::is_reference_v<T>;

/**
 * @brief Forwards to ``std::is_reference_v<T>``.
 */
template <typename T>
concept non_reference = !std::is_reference_v<T>;

/**
 * @brief Check is a type is ``void``.
 */
template <typename T>
concept is_void = std::is_void_v<T>;

/**
 * @brief Check is a type is not ``void``.
 */
template <typename T>
concept non_void = !std::is_void_v<T>;

/**
 * @brief Check if a type has ``const``, ``volatile`` or reference qualifiers.
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

/**
 * @brief Copy the ``const``/``volatile`` qualifiers from ``From`` to ``To``.
 */
template <non_reference From, unqualified To>
using forward_cv_t = typename detail::forward_cv<From, To>::type;

namespace detail {

template <typename T>
struct constify_ref;

template <typename T>
struct constify_ref<T &> : std::type_identity<T const &> {};

template <typename T>
struct constify_ref<T &&> : std::type_identity<T const &&> {};

} // namespace detail

/**
 * @brief Convert ``T & -> T const&`` and ``T && -> T const&&``.
 */
template <reference T>
using constify_ref_t = detail::constify_ref<T>::type;

/**
 * @brief True if the unqualified ``T`` and ``U`` refer to different types.
 *
 * This is useful for preventing ''T &&'' constructor/assignment from replacing the defaults.
 */
template <typename T, typename U>
concept converting = !std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>;

// ---------------- Small functions ---------------- //

/**
 * @brief Invoke a callable with the given arguments, unconditionally noexcept.
 */
template <typename... Args, std::invocable<Args...> Fn>
constexpr auto noexcept_invoke(Fn &&fun, Args &&...args) noexcept -> std::invoke_result_t<Fn, Args...> {
  return std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...);
}

// -------------------------------- //

/**
 * @brief Returns ``ptr`` and asserts it is non-null
 */
template <typename T>
auto non_null(T *ptr) noexcept -> T * {
  LF_ASSERT(ptr != nullptr);
  return ptr;
}

// -------------------------------- //

/**
 * @brief Like ``std::apply`` but reverses the argument order.
 */
template <class F, class Tuple>
constexpr auto apply_to(Tuple &&tup, F &&func) LF_HOF_RETURNS(std::apply(std::forward<F>(func), std::forward<Tuple>(tup)))

    // -------------------------------- //

    /**
     * @brief Cast a pointer to a byte pointer.
     */
    template <typename T>
    auto byte_cast(T *ptr) LF_HOF_RETURNS(std::bit_cast<forward_cv_t<T, std::byte> *>(ptr))

} // namespace lf::impl

#endif /* DF63D333_F8C0_4BBA_97E1_32A78466B8B7 */

#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef EE6A2701_7559_44C9_B708_474B1AE823B2
#define EE6A2701_7559_44C9_B708_474B1AE823B2

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <semaphore>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>


namespace lf {

inline namespace core {

// ------------------------------------------------------------------------ //

/**
 * @brief A wrapper to delay construction of an object.
 *
 * This class supports delayed construction of immovable types and reference types.
 *
 * \rst
 *
 * .. note::
 *    This documentation is generated from the non-reference specialization, see the source
 *    for the reference specialization.
 *
 * .. warning::
 *    It is undefined behavior if the object is not constructed before it is used or if the lifetime of the
 *    ``lf::eventually`` ends before an object is constructed.
 *
 * \endrst
 */
template <impl::non_void T>
class eventually;

// ------------------------------------------------------------------------ //

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

template <impl::non_void T>
  requires impl::reference<T>
class eventually<T> : impl::immovable<eventually<T>> {
 public:
  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <typename U>
    requires std::same_as<T, U &&> || std::same_as<T, impl::constify_ref_t<U &&>>
  constexpr auto operator=(U &&expr) noexcept -> eventually & {
    m_value = std::addressof(expr);
    return *this;
  }

  /**
   * @brief Access the wrapped object.
   *
   * This will decay T&& to T& just like a regular T&& function parameter.
   */
  [[nodiscard]] constexpr auto operator*() const & noexcept -> std::remove_reference_t<T> & {
    return *impl::non_null(m_value);
  }

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() const && noexcept -> T {
    if constexpr (std::is_rvalue_reference_v<T>) {
      return std::move(*impl::non_null(m_value));
    } else {
      return *impl::non_null(m_value);
    }
  }

 private:
  #ifndef NDEBUG
  std::remove_reference_t<T> *m_value = nullptr;
  #else
  std::remove_reference_t<T> *m_value;
  #endif
};

#endif

// ------------------------------------------------------------------------ //

namespace detail::static_test {

template <typename T>
using def_t = decltype(*std::declval<T>());

static_assert(std::is_assignable_v<eventually<int &>, int &>);
static_assert(not std::is_assignable_v<eventually<int &>, int &&>);
static_assert(not std::is_assignable_v<eventually<int &>, int const &>);
static_assert(not std::is_assignable_v<eventually<int &>, int const &&>);

static_assert(std::is_assignable_v<eventually<int const &>, int &>);
static_assert(not std::is_assignable_v<eventually<int const &>, int &&>);
static_assert(std::is_assignable_v<eventually<int const &>, int const &>);
static_assert(not std::is_assignable_v<eventually<int const &>, int const &&>);

static_assert(not std::is_assignable_v<eventually<int &&>, int &>);
static_assert(std::is_assignable_v<eventually<int &&>, int &&>);
static_assert(not std::is_assignable_v<eventually<int &&>, int const &>);
static_assert(not std::is_assignable_v<eventually<int &&>, int const &&>);

static_assert(not std::is_assignable_v<eventually<int const &&>, int &>);
static_assert(std::is_assignable_v<eventually<int const &&>, int &&>);
static_assert(not std::is_assignable_v<eventually<int const &&>, int const &>);
static_assert(std::is_assignable_v<eventually<int const &&>, int const &&>);

// ---------------------------------- //

static_assert(std::same_as<def_t<eventually<int &> &>, int &>);
static_assert(std::same_as<def_t<eventually<int &&> &>, int &>);
static_assert(std::same_as<def_t<eventually<int const &> &>, int const &>);
static_assert(std::same_as<def_t<eventually<int const &&> &>, int const &>);

static_assert(std::same_as<def_t<eventually<int &>>, int &>);
static_assert(std::same_as<def_t<eventually<int &&>>, int &&>);
static_assert(std::same_as<def_t<eventually<int const &>>, int const &>);
static_assert(std::same_as<def_t<eventually<int const &&>>, int const &&>);

} // namespace detail::static_test

// ------------------------------------------------------------------------ //

template <impl::non_void T>
class eventually : impl::immovable<eventually<T>> {
 public:
  // clang-format off

  /**
   * @brief Construct an empty eventually.
   */
  constexpr eventually() noexcept requires std::is_trivially_constructible_v<T> = default;

  // clang-format on

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS
  constexpr eventually() noexcept : m_init{} {};
#endif

  /**
   * @brief Construct an object inside the eventually as if by ``T(args...)``.
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

  /**
   * @brief Destroy the object which __must__ be inside the eventually.
   */
  constexpr ~eventually() noexcept requires std::is_trivially_destructible_v<T> = default;

  // clang-format on

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

  constexpr ~eventually() noexcept(std::is_nothrow_destructible_v<T>) {
  #ifndef NDEBUG
    LF_ASSUME(m_constructed);
  #endif
    std::destroy_at(std::addressof(m_value));
  }

#endif

  /**
   * @brief Access the wrapped object.
   */
  [[nodiscard]] constexpr auto operator*() & noexcept -> T & {
#ifndef NDEBUG
    LF_ASSUME(m_constructed);
#endif
    return m_value;
  }

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
    impl::empty m_init;
    T m_value;
  };

#ifndef NDEBUG
  bool m_constructed = false;
#endif
};

// ------------------------------------------------------------------------ //

namespace detail::static_test {

static_assert(std::is_assignable_v<eventually<int>, int &>);
static_assert(std::is_assignable_v<eventually<int>, int &&>);
static_assert(std::is_assignable_v<eventually<int>, int const &>);
static_assert(std::is_assignable_v<eventually<int>, int const &&>);

static_assert(std::is_assignable_v<eventually<int>, float &>);
static_assert(std::is_assignable_v<eventually<int>, float &&>);
static_assert(std::is_assignable_v<eventually<int>, float const &>);
static_assert(std::is_assignable_v<eventually<int>, float const &&>);

// ---------------------------------- //

static_assert(std::same_as<def_t<eventually<int> &>, int &>);
static_assert(std::same_as<def_t<eventually<int> &&>, int>);

} // namespace detail::static_test

} // namespace core

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */


namespace lf {

namespace impl {

/**
 * @brief A small control structure that a root task uses to communicate with the main thread.
 *
 * In general this stores an `lf::eventually` for the result and a semaphore that is
 * used to signal the main thread that the result is ready.
 */
template <typename T>
struct root_result;

/**
 * @brief A specialization of `root_result` for `void`.
 */
template <>
struct root_result<void> : immovable<root_result<void>> {
  /**
   * @brief A semaphore that is used to signal the main thread that the result is ready.
   */
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

/**
 * @brief Check if a type is a specialization of `root_result`.
 */
template <typename T>
inline constexpr bool is_root_result_v = detail::is_root_result<T>::value;

} // namespace impl

// ------------------------------------------------------------------------ //

inline namespace core {

/**
 * @brief A tuple-like type with forwarding semantics for in place construction.
 *
 * This is can be used as ``co_return in_place{...}`` to return an immovable type to an ``lf::eventually``.
 */
template <typename... Args>
struct in_place : std::tuple<Args...> {
  using std::tuple<Args...>::tuple;
};

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief A forwarding deduction guide.
 */
template <typename... Args>
in_place(Args &&...) -> in_place<Args &&...>;

#endif /* LF_DOXYGEN_SHOULD_SKIP_THIS */

} // namespace core

// ------------------------------------------------------------------------ //

inline namespace core {

namespace detail {

// General case = invalid.
template <typename R, typename T>
struct valid_result_help : std::false_type {};

// Ignore case
template <typename T>
struct valid_result_help<void, T> : std::true_type {};

// Root result special case (especially T = void)
template <typename T>
struct valid_result_help<impl::root_result<T>, T> : std::true_type {};

// Eventually special (for immovable types that cannot be assigned).
template <typename T>
struct valid_result_help<eventually<T>, T> : std::true_type {};

template <typename R, typename T>
  requires std::is_assignable_v<R &, T>
struct valid_result_help<R, T> : std::true_type {};

} // namespace detail

/**
 * @brief Verify if a coroutine that returns a ``T`` can be bound to an object of type ``R``.
 */
template <typename R, typename T>
concept valid_result = !impl::reference<R> && detail::valid_result_help<R, T>::value;

} // namespace core

// ------------------------------------------------------------------------ //

namespace impl {

/**
 * @brief A utility that stores `T*` if `T` is non-void and is otherwise empty.
 */
template <typename T>
struct maybe_ptr {
  /**
   * @brief Construct with a non-null pointer.
   */
  explicit constexpr maybe_ptr(T *ptr) noexcept : m_ptr(non_null(ptr)) {}

  /**
   * @brief Get the non-null stored pointer.
   */
  constexpr auto address() const noexcept -> T * { return m_ptr; }

 private:
  T *m_ptr;
};

template <>
struct maybe_ptr<void> {};

} // namespace impl

// ----------------------- //

inline namespace core {

/**
 * @brief A base class for promises that provides the ``return_[...]`` methods.
 *
 * This type is in the ``core`` namespace as return ``return_[...]`` methods are part of the public API.
 *
 * \rst
 *
 * In general an async function is are trying to emulate the requirements/interface of a regular function that looks like
 * this:
 *
 * .. code::
 *
 *    auto foo() -> T {
 *      // ...
 *    }
 *
 *    R val = foo();
 *
 * But really it looks like more like this:
 *
 * .. code::
 *
 *    R val;
 *
 *    val = foo()
 *
 * Hence in general we require that for ``co_await expr`` that ``expr`` is ``std::convertible_to`` ``T`` and that ``T`` is
 * ``std::is_assignable`` to ``R``. When possible we try to elide the intermediate ``T`` and assign directly to ``R``.
 *
 * .. note::
 *    This documentation is generated from the non-reference specialization, see the source
 *    for the reference specialization.
 *
 * \endrst
 *
 * @tparam R The type of the return address.
 * @tparam T The type of the return value, i.e. the `T` in `lf::task<T>`.
 */
template <typename R, typename T>
  requires valid_result<R, T>
struct promise_result;

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

// ------------------------------ void/ignore ------------------------------ //

template <>
struct promise_result<void, void> {
  static constexpr void return_void() noexcept { LF_LOG("return void"); }
};

// ------------------------------ rooted void ------------------------------ //

template <>
struct promise_result<impl::root_result<void>, void> : protected impl::maybe_ptr<impl::root_result<void>> {

  using maybe_ptr<impl::root_result<void>>::maybe_ptr;

  static constexpr void return_void() noexcept { LF_LOG("return void"); }
};

#endif

// ------------------------------ general case ------------------------------ //

template <typename R, typename T>
  requires valid_result<R, T>
struct promise_result : protected impl::maybe_ptr<R> {
 protected:
  using impl::maybe_ptr<R>::maybe_ptr;

 public:
  /**
   * @brief Assign `value` to the return address.
   */
  constexpr void return_value(T const &value) const
    requires std::convertible_to<T const &, T> && impl::non_reference<T>
  {
    if constexpr (impl::non_void<R>) {
      *(this->address()) = value;
    }
  }

  /**
   * @brief Move assign `value` to the return address.
   */
  constexpr void return_value(T &&value) const
    requires std::convertible_to<T &&, T>
  {
    if constexpr (impl::non_void<R>) {
      if constexpr (std::is_rvalue_reference_v<T &&>) {
        *(this->address()) = std::move(value);
      } else {
        *(this->address()) = value;
      }
    }
  }

  /**
   * @brief Convert and assign `value` to the return address.
   *
   * If the return address is directly assignable from `value` this will not construct the intermediate `T`.
   */
  template <std::convertible_to<T> U>
    requires impl::converting<T, U>
  constexpr void return_value(U &&value) const {
    if constexpr (impl::non_void<R>) {
      if constexpr (std::is_assignable_v<R &, U &&>) {
        *(this->address()) = std::forward<U>(value);
      } else {
        *(this->address()) = [&]() -> T { return std::forward<U>(value); }();
      }
    }
  }

 private:
  template <typename U>
  using strip_rvalue_ref_t = std::conditional_t<std::is_rvalue_reference_v<U>, std::remove_reference_t<U>, U>;

 public:
  /**
   * @brief Assign a value constructed from the arguments stored in `args` to the return address.
   *
   * If the return address has an `.emplace()` method that accepts the arguments in the tuple this will be
   * called directly.
   */
  template <impl::reference... Args>
    requires std::constructible_from<T, Args...>
  constexpr void return_value(in_place<Args...> args) const {

#define LF_FWD_ARGS std::forward<strip_rvalue_ref_t<Args>>(args)...

    if constexpr (impl::non_void<R>) {
      impl::apply_to(static_cast<std::tuple<Args...> &&>(args), [ret = this->address()](Args... args) {
        if constexpr (requires { ret->emplace(LF_FWD_ARGS); }) {
          ret->emplace(LF_FWD_ARGS);
        } else {
          (*ret) = T(LF_FWD_ARGS);
        }
      });
    }
  }

#undef LF_FWD_ARGS
};

} // namespace core

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
#elif __has_include(<experimental/coroutine>) // Check for an experimental version.
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

// -------------------- async stack -------------------- //

inline namespace ext {

class async_stack;

} // namespace ext

namespace impl {

static constexpr std::size_t k_stack_size = k_kibibyte * LF_ASYNC_STACK_SIZE;

/**
 * @brief Get a pointer to the end of a stack's buffer.
 */
auto stack_as_bytes(async_stack *stack) noexcept -> std::byte *;
/**
 * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
 */
auto bytes_to_stack(std::byte *bytes) noexcept -> async_stack *;

} // namespace impl

inline namespace ext {

/**
 * @brief A fraction of a thread's cactus stack.
 */
class async_stack : impl::immovable<async_stack> {
  alignas(impl::k_new_align) std::byte m_buf[impl::k_stack_size]; // NOLINT

  friend auto impl::stack_as_bytes(async_stack *stack) noexcept -> std::byte *;

  friend auto impl::bytes_to_stack(std::byte *bytes) noexcept -> async_stack *;
};

static_assert(std::is_standard_layout_v<async_stack>);
static_assert(sizeof(async_stack) == impl::k_stack_size, "Spurious padding in async_stack!");

} // namespace ext

namespace impl {

inline auto stack_as_bytes(async_stack *stack) noexcept -> std::byte * { return std::launder(stack->m_buf + k_stack_size); }

inline auto bytes_to_stack(std::byte *bytes) noexcept -> async_stack * {
#ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(std::is_pointer_interconvertible_with_class(&async_stack::m_buf));
#endif
  return std::launder(std::bit_cast<async_stack *>(bytes - k_stack_size));
}

} // namespace impl

// ---------------------------------------- //

inline namespace ext {

struct frame_block;

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

} // namespace ext

// ----------------------------------------------- //

/**
 * @brief This namespace contains `inline thread_local constinit` variables and functions to manipulate them.
 */
namespace impl::tls {

template <thread_context Context>
constinit inline thread_local Context *ctx = nullptr;

constinit inline thread_local std::byte *asp = nullptr;

} // namespace impl::tls

// ----------------------------------------------- //

namespace impl {

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

} // namespace impl

// ----------------------------------------------- //

inline namespace ext {

/**
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */
struct frame_block : private impl::immovable<frame_block>, impl::debug_block {
  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks.
   */
  void resume_stolen() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(impl::tls::asp);
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

// protected:
/**
 * @brief Construct a frame block.
 *
 * Pass ``top == nullptr`` if this is on the heap. Non-root tasks will need to call ``set_parent(...)``.
 */
#ifndef LF_COROUTINE_OFFSET
  constexpr frame_block(stdx::coroutine_handle<> coro, std::byte *top) : m_coro{coro}, m_top(top) { LF_ASSERT(coro); }
#else
  constexpr frame_block(stdx::coroutine_handle<>, std::byte *top) : m_top(top) {}
#endif

  /**
   * @brief Set the pointer to the parent frame.
   */
  void set_parent(frame_block *parent) noexcept {
    LF_ASSERT(!m_parent);
    m_parent = impl::non_null(parent);
  }

  /**
   * @brief Get a pointer to the parent frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame_block * { return impl::non_null(m_parent); }

  /**
   * @brief Get a pointer to the top of the top of the async-stack this frame was allocated on.
   */
  [[nodiscard]] auto top() const noexcept -> std::byte * { return impl::non_null(m_top); }

  /**
   * @brief Get the coroutine handle for this frames coroutine.
   */
  [[nodiscard]] auto coro() noexcept -> stdx::coroutine_handle<> {
#ifndef LF_COROUTINE_OFFSET
    return m_coro;
#else
    return stdx::coroutine_handle<>::from_address(byte_cast(this) - LF_COROUTINE_OFFSET);
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
   * @brief Check if this is a root frame.
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
    std::construct_at(&m_join, impl::k_u32_max);
  }

 private:
#ifndef LF_COROUTINE_OFFSET
  stdx::coroutine_handle<> m_coro;
#endif

  std::byte *m_top;                              ///< Needs to be separate in-case allocation elided.
  frame_block *m_parent = nullptr;               ///< Same ^
  std::atomic_uint32_t m_join = impl::k_u32_max; ///< Number of children joined (with offset).
  std::uint32_t m_steal = 0;                     ///< Number of steals.
};

static_assert(alignof(frame_block) <= impl::k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

} // namespace ext

// ----------------------------------------------- //

namespace impl::tls {

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
  async_stack *stack = bytes_to_stack(prev);
  ctx<Context>->stack_push(stack);
}

} // namespace impl::tls

// ----------------------------------------------- //

inline namespace ext {

template <thread_context Context>
inline void frame_block::resume_external() noexcept {

  LF_LOG("Call to resume on external task");

  LF_ASSERT(impl::tls::asp);

  if (!is_root()) {
    impl::tls::eat<Context>(top());
  } else {
    LF_LOG("External was root");
  }

  coro().resume();

  LF_ASSERT(impl::tls::asp);
}

} // namespace ext

namespace impl {

// ----------------------------------------------- //

/**
 * @brief A base class for promises that allocates on the heap.
 */
struct promise_alloc_heap : frame_block {
 protected:
  explicit promise_alloc_heap(stdx::coroutine_handle<> self) noexcept : frame_block{self, nullptr} {}
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
  //   return std::max(align, impl::k_new_align);
  // }

 protected:
  explicit promise_alloc_stack(stdx::coroutine_handle<> self) noexcept : frame_block{self, tls::asp} {}

 public:
  /**
   * @brief Allocate the coroutine on the current `async_stack`.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) -> void * {
    LF_ASSERT(tls::asp);
    tls::asp -= (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Allocating {} bytes on stack from {}", size, (void *)tls::asp);
    return tls::asp;
  }

  /**
   * @brief Deallocate the coroutine on the current `async_stack`.
   */
  static void operator delete(void *ptr, std::size_t size) {
    LF_ASSERT(ptr == tls::asp);
    tls::asp += (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Deallocating {} bytes on stack to {}", size, (void *)tls::asp);
  }
};

} // namespace impl

inline namespace ext {

/**
 * @brief Initialize thread-local variables before a worker can resume submitted tasks.
 *
 * .. warning::
 *    These should be cleaned up with `worker_finalize(...)`.
 */
template <thread_context Context>
void worker_init(Context *context) {

  LF_ASSERT(context);
  LF_ASSERT(!impl::tls::ctx<Context>);
  LF_ASSERT(!impl::tls::asp);

  impl::tls::ctx<Context> = context;
  impl::tls::asp = impl::stack_as_bytes(context->stack_pop());
}

/**
 * @brief Clean-up thread-local variable before destructing a worker's context.
 *
 * .. warning::
 *    These must be initialized with `worker_init(...)`.
 */
template <thread_context Context>
void worker_finalize(Context *context) {
  LF_ASSERT(context == impl::tls::ctx<Context>);
  LF_ASSERT(impl::tls::asp);

  context->stack_push(impl::bytes_to_stack(impl::tls::asp));

  impl::tls::asp = nullptr;
  impl::tls::ctx<Context> = nullptr;
}

} // namespace ext

} // namespace lf

#endif /* C5C3AA77_D533_4A89_8D33_99BD819C1B4C */


/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

inline namespace core {

/**
 * @brief A fixed string type for template parameters that tracks its source location.
 */
template <typename Char, std::size_t N>
struct tracked_fixed_string {
 private:
  using sloc = std::source_location;
  static constexpr std::size_t file_name_max_size = 127;

 public:
  /**
   * @brief Construct a tracked fixed string from a string literal.
   */
  consteval tracked_fixed_string(Char const (&str)[N], sloc loc = sloc::current()) noexcept
      : line{loc.line()},
        column{loc.column()} {
    for (std::size_t i = 0; i < N; ++i) {
      function_name[i] = str[i];
    }

    // std::size_t count = 0 loc.while
  }

  // std::array<char, file_name_max_size + 1> file_name_buf;
  // std::size_t file_name_size;

  std::array<Char, N> function_name; ///< The given name of the function.
  std::uint_least32_t line;          ///< The line number where `this` was constructed.
  std::uint_least32_t column;        ///< The column number where `this` was constructed.
};

} // namespace core

// ----------------------------------------------- //

inline namespace core {

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `lf` coroutines from other coroutines.
 *
 * \rst
 *
 * .. warning::
 *    The value type ``T`` of a coroutine should never be a function of its context or return address type.
 *
 * \endrst
 */
template <typename T = void, tracked_fixed_string Name = "">
struct task {
  using value_type = T; ///< The type of the value returned by the coroutine.

  /**
   * @brief Construct a new task object.
   *
   * This should only be called by the compiler.
   */
  task(frame_block *frame) : m_frame{non_null(frame)} {}

  /**
   * @brief __Not__ part of the public API.
   */
  [[nodiscard]] constexpr auto frame() const noexcept -> frame_block * { return m_frame; }

 private:
  frame_block *m_frame; ///< The frame block for the coroutine.
};

} // namespace core

namespace impl {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T, auto Name>
struct is_task_impl<task<T, Name>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

} // namespace impl

inline namespace core {

// ----------------------------------------------- //

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 *
 * You can inspect the first arg of an async function to determine the tag.
 */
enum class tag {
  root,   ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call,   ///< Non root task (on a virtual stack) from an ``lf::call``, completes synchronously.
  fork,   ///< Non root task (on a virtual stack) from an ``lf::fork``, completes asynchronously.
  invoke, ///< Equivalent to ``lf::call`` but caches the return (extra move required).
  tail,   ///< Force a [tail-call](https://en.wikipedia.org/wiki/Tail_call) optimization.
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
 * @brief Check if an invocable is suitable for use as an `lf::async` function.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

// ----------------------------------------------- //

template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async;

/**
 * @brief The API of the first argument passed to an async function.
 *
 * All async functions must have a templated first arguments, this argument
 * will be generated by the compiler and encodes many useful/queryable properties.
 *
 * \rst
 *
 * .. include:: ../../include/libfork/core/task.hpp
 *    :code:
 *    :start-line: 206
 *    :end-line: 232
 *
 * \endrst
 */
template <typename Arg>
concept first_arg = impl::unqualified<Arg> && requires(Arg arg) {
  //

  requires std::is_trivially_copyable_v<Arg>;

  tag_of<Arg>;

  typename context_of<Arg>;
  typename return_of<Arg>;
  typename function_of<Arg>;

  requires !std::is_reference_v<return_of<Arg>>;

  { std::remove_cvref_t<Arg>::context() } -> std::same_as<context_of<Arg> *>;

  requires impl::is_void<return_of<Arg>> || requires {
    { arg.address() } -> std::convertible_to<return_of<Arg> *>;
  };

  []<typename F>(async<F>)
    requires std::same_as<F, function_of<Arg>>
  {
    // Check Args is implicitly convertible to the appropriate async<?>.
  }
  (arg);
};

} // namespace core

namespace impl {

// ----------------------------------------------- //

template <typename T>
concept not_first_arg = !first_arg<T>;

// ----------------------------------------------- //

template <typename Task, typename Head>
concept valid_return = is_task<Task> && valid_result<return_of<Head>, value_of<Task>>;

/**
 * @brief Check that the async function encoded in `Head` is invocable with arguments in `Tail`.
 */
template <typename Head, typename... Tail>
concept valid_packet = first_arg<Head> && valid_return<std::invoke_result_t<function_of<Head>, Head, Tail...>, Head>;

/**
 * @brief A base class for building the first argument to asynchronous functions.
 *
 * This derives from `async<F>` to allow to allow for use as a y-combinator.
 *
 * It needs the true context type to be patched to it.
 *
 * This is used by `stdx::coroutine_traits` to build the promise type.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg;

/**
 * @brief A helper to statically attach a new `context_type` to a `first_arg`.
 */
template <thread_context Context, first_arg Head>
struct patched : Head {

  using context_type = Context;

  /**
   * @brief Get a pointer to the thread-local context.
   *
   * \rst
   *
   * .. warning::
   *    This may change at every ``co_await``!
   *
   * \endrst
   */
  [[nodiscard]] static auto context() -> Context * { return non_null(tls::ctx<Context>); }
};

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
class [[nodiscard("packets must be co_awaited")]] packet : move_only<packet<Head, Tail...>> {
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
  constexpr packet(Head head, Tail &&...tail) noexcept : m_args{std::move(head), std::forward<Tail>(tail)...} {}

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

} // namespace impl

// ----------------------------------------------- //

inline namespace core {

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 *
 * Use this type to define an synchronous function.
 */
template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async {
  /**
   * @brief For use with an explicit template-parameter.
   */
  consteval async() = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide
   * generated from this.
   *
   * \rst
   *
   * This is to allow concise definitions from lambdas:
   * .. code::
   *
   *    constexpr async fib = [](auto fib, ...) -> task<int, "fib"> {
   *        // ...
   *    };
   *
   * \endrst
   */
  consteval async([[maybe_unused]] Fn invocable_which_returns_a_task) {}

 private:
  template <typename... Args>
  using invoke_packet = impl::packet<impl::basic_first_arg<void, tag::invoke, Fn>, Args...>;

  template <typename... Args>
  using call_packet = impl::packet<impl::basic_first_arg<void, tag::call, Fn>, Args...>;

 public:
  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <typename... Args>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept -> invoke_packet<Args...> {
    return {{}, std::forward<Args>(args)...};
  }

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   */
  template <typename... Args>
    requires impl::is_void<value_of<invoke_packet<Args...>>>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept -> call_packet<Args...> {
    return {{}, std::forward<Args>(args)...};
  }

#endif
};

} // namespace core

// ----------------------------------------------- //

namespace impl {

/**
 * @brief A type that satisfies the ``thread_context`` concept.
 *
 * This is used to detect bad coroutine calls early. All its methods are
 * unimplemented as it is only used in unevaluated contexts.
 */
struct dummy_context {
  auto max_threads() -> std::size_t;      ///< Unimplemented.
  auto submit(frame_block *) -> void;     ///< Unimplemented.
  auto task_pop() -> frame_block *;       ///< Unimplemented.
  auto task_push(frame_block *) -> void;  ///< Unimplemented.
  auto stack_pop() -> async_stack *;      ///< Unimplemented.
  auto stack_push(async_stack *) -> void; ///< Unimplemented.
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

/**
 * @brief Void/ignore specialization.
 */
template <tag Tag, stateless F>
struct basic_first_arg<void, Tag, F> : async<F>, private move_only<basic_first_arg<void, Tag, F>> {
  using context_type = dummy_context;   ///< A default context
  using return_type = void;             ///< The type of the return address.
  using function_type = F;              ///< The underlying async
  static constexpr tag tag_value = Tag; ///< The tag value.

  /**
   * @brief Unimplemented - to satisfy the ``thread_context`` concept.
   */
  [[nodiscard]] static auto context() -> context_type * { LF_THROW(std::runtime_error{"Should never be called!"}); }
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg : basic_first_arg<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  constexpr basic_first_arg(return_type &ret) : m_ret{std::addressof(ret)} {}

  [[nodiscard]] constexpr auto address() const noexcept -> return_type * { return m_ret; }

 private:
  R *m_ret;
};

static_assert(first_arg<basic_first_arg<void, tag::root, decltype([] {})>>);
static_assert(first_arg<basic_first_arg<int, tag::root, decltype([] {})>>);

} // namespace impl

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */


/**
 * @file call.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` machinery.
 */

namespace lf {

namespace impl {

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

/**
 * @brief A empty tag type used to disambiguate a join.
 */
struct join_type {};

} // namespace impl

inline namespace core {

/**
 * @brief An awaitable (in a `lf::task`) that triggers a join.
 */
inline constexpr impl::join_type join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 */
inline constexpr impl::bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr impl::bind_task<tag::call> call = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a
 * [tail-call](https://en.wikipedia.org/wiki/Tail_call).
 */
inline constexpr impl::bind_task<tag::tail> tail = {};

} // namespace core

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
#include <type_traits>
#include <utility>



/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::impl {

// -------------------------------------------------------------------------- //

/**
 * @brief Selects the allocator used by `promise_type` depending on tag.
 */
template <tag Tag>
using allocator = std::conditional_t<Tag == tag::root, promise_alloc_heap, promise_alloc_stack>;

/**
 * @brief The promise type for all tasks/coroutines.
 *
 * @tparam R The type of the return address.
 * @tparam T The value type of the coroutine (what it promises to return).
 * @tparam Context The type of the context this coroutine is running on.
 * @tparam Tag The dispatch tag of the coroutine.
 */
template <typename R, typename T, thread_context Context, tag Tag>
struct promise_type : allocator<Tag>, promise_result<R, T> {
 private:
  static_assert(Tag == tag::fork || Tag == tag::call || Tag == tag::root);
  static_assert(Tag != tag::root || is_root_result_v<R>);

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

      if constexpr (Tag == tag::call) {
        LF_LOG("Inline task resumes parent");
        // Inline task's parent cannot have been stolen, no need to reset control block.
        return parent->coro();
      }

      Context *context = non_null(tls::ctx<Context>);

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
        tls::asp = stack_as_bytes(context->stack_pop());
      }

      return stdx::noop_coroutine();
    }
  };

  struct fork_awaitable : stdx::suspend_always {
    constexpr auto await_suspend(stdx::coroutine_handle<promise_type> parent) noexcept -> stdx::coroutine_handle<> {
      LF_LOG("Forking, push parent to context");
      LF_ASSERT(&parent.promise() == m_parent);
      // Need it here (on real stack) in case *this is destructed after push.
      stdx::coroutine_handle child = m_child->coro();
      tls::ctx<Context>->task_push(m_parent);
      return child;
    }
    frame_block *m_parent;
    frame_block *m_child;
  };

  struct call_awaitable : stdx::suspend_always {
    constexpr auto await_suspend(stdx::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {
      LF_LOG("Calling");
      return m_child->coro();
    }
    frame_block *m_child;
  };

  struct join_awaitable {
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
    constexpr auto await_ready() const noexcept -> bool {
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

    constexpr auto await_suspend(stdx::coroutine_handle<promise_type> task) noexcept -> stdx::coroutine_handle<> {
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

 public:
  /**
   * @brief Construct promise with void return type.
   */
  constexpr promise_type() noexcept : allocator<Tag>(stdx::coroutine_handle<promise_type>::from_promise(*this)) {}

  /**
   * @brief Construct promise, sets return address.
   */
  template <first_arg Head, typename... Tail>
  explicit constexpr promise_type(Head const &head, Tail const &...) noexcept
    requires std::constructible_from<promise_result<R, T>, R *>
      : allocator<Tag>{stdx::coroutine_handle<promise_type>::from_promise(*this)},
        promise_result<R, T>{head.address()} {}

  /**
   * @brief Construct promise, sets return address.
   *
   * For member function coroutines.
   */
  template <not_first_arg Self, first_arg Head, typename... Tail>
  explicit constexpr promise_type(Self const &, Head const &head, Tail const &...tail) noexcept
    requires std::constructible_from<promise_result<R, T>, R *>
      : promise_type{head, tail...} {}

  auto get_return_object() noexcept -> frame_block * { return this; }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  /**
   * @brief Terminates the program.
   */
  void unhandled_exception() noexcept {
    noexcept_invoke([] { LF_RETHROW; });
  }

  auto final_suspend() noexcept -> final_awaitable {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the async_stack this child is on

    LF_ASSERT(this->debug_count() == 0);
    LF_ASSERT(this->steals() == 0);                                      // Fork without join.
    LF_ASSERT(this->load_joins(std::memory_order_relaxed) == k_u32_max); // Destroyed in invalid state.

    return final_awaitable{};
  }

  /**
   * @brief Transform a fork packet into a fork awaitable.
   */
  template <first_arg Head, typename... Args>
    requires(tag_of<Head> == tag::fork)
  constexpr auto await_transform(packet<Head, Args...> &&packet) noexcept -> fork_awaitable {
    this->debug_inc();
    return {{}, this, std::move(packet).template patch_with<Context>().invoke(this)};
  }

  /**
   * @brief Transform a call packet into a call awaitable.
   */
  template <first_arg Head, typename... Args>
    requires(tag_of<Head> == tag::call)
  constexpr auto await_transform(packet<Head, Args...> &&packet) noexcept -> call_awaitable {
    return {{}, std::move(packet).template patch_with<Context>().invoke(this)};
  }

  /**
   * @brief Transform an invoke packet into a call awaitable.
   */
  template <typename F, typename... Args>
  constexpr auto await_transform(packet<basic_first_arg<void, tag::invoke, F>, Args...> &&packet) {

    using packet_t = impl::packet<basic_first_arg<void, tag::invoke, F>, Args...>;

    static_assert(non_void<value_of<packet_t>>, "async's call op should prevent this");

    using new_packet_t = impl::packet<basic_first_arg<eventually<value_of<packet_t>>, tag::call, F>, Args...>;

    struct awaitable : stdx::suspend_always {
      constexpr auto await_suspend(stdx::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

        new_packet_t new_packet = std::move(m_packet).apply([&](auto, Args &&...args) -> new_packet_t {
          return {{m_res}, std::forward<Args>(args)...};
        });

        static_assert(std::same_as<value_of<packet_t>, value_of<new_packet_t>>, "value_type dependent on first arg!");

        LF_LOG("Invoking");
        return std::move(new_packet).template patch_with<Context>().invoke(parent)->coro();
      }

      [[nodiscard]] constexpr auto await_resume() -> value_of<packet_t> { return *std::move(m_res); }

      frame_block *parent;
      packet_t m_packet;
      eventually<value_of<packet_t>> m_res;
    };

    return awaitable{{}, this, std::move(packet), {}};
  }

  constexpr auto await_transform(join_type) noexcept -> join_awaitable { return {this}; }
};

} // namespace lf::impl

namespace lf::impl {

/**
 * @brief Disable rvalue references for T&& template types if an async function is forked.
 *
 * This is to prevent the user from accidentally passing a temporary object to
 * an async function that will then destructed in the parent task before the
 * child task returns.
 */
template <typename T, tag Tag>
concept no_dangling = Tag != tag::fork || !std::is_rvalue_reference_v<T>;

namespace detail {

template <first_arg Head, is_task Task>
using promise_for = impl::promise_type<return_of<Head>, value_of<Task>, context_of<Head>, tag_of<Head>>;

} // namespace detail

} // namespace lf::impl

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::impl::is_task Task, lf::first_arg Head, lf::impl::no_dangling<lf::tag_of<Head>>... Args>
struct lf::stdx::coroutine_traits<Task, Head, Args...> {
  using promise_type = lf::impl::detail::promise_for<Head, Task>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::impl::is_task Task, lf::impl::not_first_arg This, lf::first_arg Head,
          lf::impl::no_dangling<lf::tag_of<Head>>... Args>
struct lf::stdx::coroutine_traits<Task, This, Head, Args...> : lf::stdx::coroutine_traits<Task, Head, Args...> {};

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */
#ifndef E54125F4_034E_45CD_8DF4_7A71275A5308
#define E54125F4_034E_45CD_8DF4_7A71275A5308

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>



namespace lf {

inline namespace ext {

/**
 * @brief A concept that schedulers must satisfy.
 */
template <typename Sch>
concept scheduler = requires(Sch &&sch, frame_block *ext) {
  typename context_of<Sch>;
  std::forward<Sch>(sch).submit(ext);
};

} // namespace ext

inline namespace core {

namespace detail {

template <typename Context, stateless F, typename... Args>
struct sync_wait_impl {

  template <typename R>
  using first_arg_t = impl::patched<Context, impl::basic_first_arg<R, tag::root, F>>;

  using dummy_packet = impl::packet<first_arg_t<void>, Args...>;
  using dummy_packet_value_type = value_of<std::invoke_result_t<F, dummy_packet, Args...>>;

  using real_packet = impl::packet<first_arg_t<impl::root_result<dummy_packet_value_type>>, Args...>;
  using real_packet_value_type = value_of<std::invoke_result_t<F, real_packet, Args...>>;

  static_assert(std::same_as<dummy_packet_value_type, real_packet_value_type>, "Value type changes!");
};

template <scheduler Sch, stateless F, typename... Args>
using result_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet_value_type;

template <scheduler Sch, stateless F, typename... Args>
using packet_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet;

} // namespace detail

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 *
 * This is a blocking call to an `lf::async` function. The function will be executed on `sch`.
 *
 * \rst
 *
 * .. warning::
 *
 *    This should never be called from within a coroutine.
 *
 * \endrst
 */
template <scheduler Sch, stateless F, class... Args>
auto sync_wait(Sch &&sch, [[maybe_unused]] async<F> fun, Args &&...args) noexcept -> detail::result_t<Sch, F, Args...> {

  impl::root_result<detail::result_t<Sch, F, Args...>> root_block;

  detail::packet_t<Sch, F, Args...> packet{{{root_block}}, std::forward<Args>(args)...};

  frame_block *ext = std::move(packet).invoke();

  LF_LOG("Submitting root");

  std::forward<Sch>(sch).submit(ext);

  LF_LOG("Acquire semaphore");

  root_block.semaphore.acquire();

  LF_LOG("Semaphore acquired");

  if constexpr (impl::non_void<detail::result_t<Sch, F, Args...>>) {
    return *std::move(root_block);
  }
}

} // namespace core

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
