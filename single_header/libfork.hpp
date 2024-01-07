
//---------------------------------------------------------------//
//        This is a machine generated file DO NOT EDIT IT        //
//---------------------------------------------------------------//

#ifndef EDCA974A_808F_4B62_95D5_4D84E31B8911
#define EDCA974A_808F_4B62_95D5_4D84E31B8911

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef B3512749_D678_438A_8E60_B1E880CF6C23
#define B3512749_D678_438A_8E60_B1E880CF6C23

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef B13463FB_3CF9_46F1_AFAC_19CBCB99A23C
#define B13463FB_3CF9_46F1_AFAC_19CBCB99A23C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <source_location>
#include <type_traits>
#ifndef A6BE090F_9077_40E8_9B57_9BAFD9620469
#define A6BE090F_9077_40E8_9B57_9BAFD9620469

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef A951FB73_0FCF_4B7C_A997_42B7E87D21CB
#define A951FB73_0FCF_4B7C_A997_42B7E87D21CB
// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <memory>
#include <span>
#ifndef CF97E524_27A6_4CD9_8967_39F1B1BE97B6
#define CF97E524_27A6_4CD9_8967_39F1B1BE97B6

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stdexcept>
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
 * @brief A collection of internal/public macros.
 *
 * These are exhaustively documented due to macros nasty visibility rules however, only
 * macros that are marked as __[public]__ should be consumed.
 */

// NOLINTBEGIN Sometime macros are the only way to do things...

/**
 * @brief __[public]__ The major version of libfork.
 *
 * Changes with incompatible API changes.
 */
#define LF_VERSION_MAJOR 3
/**
 * @brief __[public]__ The minor version of libfork.
 *
 * Changes when functionality is added in an API backward compatible manner.
 */
#define LF_VERSION_MINOR 5
/**
 * @brief __[public]__ The patch version of libfork.
 *
 * Changes when bug fixes are made in an API backward compatible manner.
 */
#define LF_VERSION_PATCH 0

/**
 * @brief Use to conditionally decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) with
 * ``static``.
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
 * @brief __[public]__ Detects if the compiler has exceptions enabled.
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

namespace lf::impl {

#ifdef __cpp_lib_unreachable
using std::unreachable;
#else
/**
 * @brief A homebrew version of `std::unreachable`, see https://en.cppreference.com/w/cpp/utility/unreachable
 */
[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  // Even if no extension is used, undefined behavior is still raised by
  // an empty function body and the noreturn attribute.
  #if defined(_MSC_VER) && !defined(__clang__) // MSVC
  __assume(false);
  #else                                        // GCC, Clang
  __builtin_unreachable();
  #endif
}
#endif

} // namespace lf::impl

/**
 * @brief Invokes undefined behavior if ``expr`` evaluates to `false`.
 *
 * \rst
 *
 *  .. warning::
 *
 *    This has different semantics than ``[[assume(expr)]]`` as it WILL evaluate the
 *    expression at runtime. Hence you should conservatively only use this macro
 *    if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */

#define LF_ASSUME(expr)                                                                                      \
  do {                                                                                                       \
    if (!(expr)) {                                                                                           \
      ::lf::impl::unreachable();                                                                             \
    }                                                                                                        \
  } while (false)

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  `` `` otherwise ``assert(expr)``.
 *
 * This is for expressions with side-effects.
 */
#ifndef NDEBUG
  #define LF_ASSERT_NO_ASSUME(expr) assert(expr)
#else
  #define LF_ASSERT_NO_ASSUME(expr)                                                                          \
    do {                                                                                                     \
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
 * @brief Macro to prevent a function to be inlined.
 */
#if !defined(LF_NOINLINE)
  #if defined(_MSC_VER)
    #define LF_NOINLINE __declspec(noinline)
  #elif defined(__GNUC__) && __GNUC__ > 3
  // Clang also defines __GNUC__ (as 4)
    #if defined(__CUDACC__)
  // nvcc doesn't always parse __noinline__, see: https://svn.boost.org/trac/boost/ticket/9392
      #define LF_NOINLINE __attribute__((noinline))
    #elif defined(__HIP__)
  // See https://github.com/boostorg/config/issues/392
      #define LF_NOINLINE __attribute__((noinline))
    #else
      #define LF_NOINLINE __attribute__((__noinline__))
    #endif
  #else
    #define LF_NOINLINE
  #endif
#endif

/**
 * @brief Macro to use next to 'inline' to force a function to be inlined.
 *
 * \rst
 *
 * .. note::
 *
 *    This does not imply the c++'s `inline` keyword which also has an effect on linkage.
 *
 * \endrst
 */
#if !defined(LF_FORCEINLINE)
  #if defined(_MSC_VER)
    #define LF_FORCEINLINE __forceinline
  #elif defined(__GNUC__) && __GNUC__ > 3
  // Clang also defines __GNUC__ (as 4)
    #define LF_FORCEINLINE __attribute__((__always_inline__))
  #else
    #define LF_FORCEINLINE
  #endif
#endif

/**
 * @brief Compiler specific attributes libfork uses for its coroutine types.
 */
#if defined(__clang__) && defined(__has_attribute)
  #if __has_attribute(coro_return_type) && __has_attribute(coro_only_destroy_when_complete)
    #define LF_CORO_ATTRIBUTES [[clang::coro_only_destroy_when_complete]] [[clang::coro_return_type]]
  #else
    #define LF_CORO_ATTRIBUTES
  #endif
#else
  #define LF_CORO_ATTRIBUTES
#endif

/**
 * @brief __[public]__ A customizable logging macro.
 *
 * By default this is a no-op. Defining ``LF_DEFAULT_LOGGING`` will enable a default
 * logging implementation which prints to ``std::cout``. Overridable by defining your
 * own ``LF_LOG`` macro with an API like ``std::format()``.
 */
#ifndef LF_LOG
  #ifdef LF_DEFAULT_LOGGING
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

    #define LF_LOG(message, ...)                                                                             \
      do {                                                                                                   \
        if (!std::is_constant_evaluated()) {                                                                 \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n';                     \
        }                                                                                                    \
      } while (false)
  #else
    #define LF_LOG(head, ...)
  #endif
#endif

/**
 * @brief Concatenation macro
 */
#define LF_CONCAT_OUTER(a, b) LF_CONCAT_INNER(a, b)
/**
 * @brief Internal concatenation macro (use LF_CONCAT_OUTER)
 */
#define LF_CONCAT_INNER(a, b) a##b

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */

#ifndef D66BBECE_E467_4EB6_B74A_AAA2E7256E02
#define D66BBECE_E467_4EB6_B74A_AAA2E7256E02

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <version>
#ifndef A75DC3F0_D0C3_4669_A901_0B22556C873C
#define A75DC3F0_D0C3_4669_A901_0B22556C873C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * @file tag.hpp
 *
 * @brief Libfork's dispatch tag types.
 */

namespace lf {

inline namespace core {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 *
 * You can inspect the first argument of an async function to determine the tag.
 */
enum class tag {
  root, ///< This coroutine is a root task from an ``lf::sync_wait``.
  call, ///< Non root task from an ``lf::call``, completes synchronously.
  fork, ///< Non root task from an ``lf::fork``, completes asynchronously.
};

} // namespace core

} // namespace lf

#endif /* A75DC3F0_D0C3_4669_A901_0B22556C873C */

#ifndef C9703881_3D9C_41A5_A7A2_44615C4CFA6A
#define C9703881_3D9C_41A5_A7A2_44615C4CFA6A

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
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#ifndef DF63D333_F8C0_4BBA_97E1_32A78466B8B7
#define DF63D333_F8C0_4BBA_97E1_32A78466B8B7

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <new>
#include <optional>
#include <source_location>
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
 * This is the namespace that contains the minimal user-facing API of ``libfork``, notably
 * this excludes schedulers and algorithms.
 */
inline namespace core {}

/**
 * @brief An inline namespace that wraps extension functionality.
 *
 * This namespace is part of ``libfork``s public API but is intended for advanced users
 * writing schedulers, It exposes the scheduler/context API's alongside some implementation
 * details (such as lock-free dequeues, a `hwloc` abstraction, and synchronization primitives)
 * that could be useful when implementing custom schedulers.
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
static constexpr std::uint16_t k_u16_max = std::numeric_limits<std::uint16_t>::max();

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
 * @brief A functor that returns ``std::nullopt``.
 */
template <typename T>
struct return_nullopt {
  LF_STATIC_CALL constexpr auto operator()() LF_STATIC_CONST noexcept -> std::optional<T> { return {}; }
};

// -------------------------------- //

/**
 * @brief An empty base class that is not copyable or movable.
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
 * @brief Test if we can form a reference to an instance of type T.
 */
template <typename T>
concept can_reference = requires () { typename std::type_identity_t<T &>; };

// Ban constructs like (T && -> T const &) which would dangle.

namespace detail {

template <typename To, typename From>
struct safe_ref_bind_impl : std::false_type {};

// All reference types can bind to a non-dangling reference of the same kind without dangling.

template <typename T>
struct safe_ref_bind_impl<T, T> : std::true_type {};

// `T const X` can additionally bind to `T X` without dangling//

template <typename To, typename From>
  requires (!std::same_as<To const &, From &>)
struct safe_ref_bind_impl<To const &, From &> : std::true_type {};

template <typename To, typename From>
  requires (!std::same_as<To const &&, From &&>)
struct safe_ref_bind_impl<To const &&, From &&> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that ``To expr = From`` is valid and does not dangle.
 *
 * This requires that ``To`` and ``From`` are both the same reference type or that ``To`` is a
 * const qualified version of ``From``. This explicitly bans conversions like ``T && -> T const &``
 * which would dangle.
 */
template <typename From, typename To>
concept safe_ref_bind_to =                          //
    std::is_reference_v<To> &&                      //
    can_reference<From> &&                          //
    detail::safe_ref_bind_impl<To, From &&>::value; //

namespace detail {

template <class Lambda, int = (((void)Lambda{}()), 0)>
consteval auto constexpr_callable_help(Lambda) -> bool {
  return true;
}

consteval auto constexpr_callable_help(auto &&...) -> bool { return false; }

} // namespace detail

/**
 * @brief Detect if a function is constexpr-callable.
 *
 * \rst
 *
 * Use like:
 *
 * .. code::
 *
 *    if constexpr (is_constexpr<[]{ function_to_test() }>){
 *      // ...
 *    }
 *
 * \endrst
 */
template <auto Lambda>
concept constexpr_callable = detail::constexpr_callable_help(Lambda);

namespace detail::static_test {

inline void foo() {}

static_assert(constexpr_callable<[] {}>);

static_assert(constexpr_callable<[] {
  std::has_single_bit(1U);
}>);

static_assert(!constexpr_callable<[] {
  foo();
}>);

} // namespace detail::static_test

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
using constify_ref_t = typename detail::constify_ref<T>::type;

/**
 * @brief True if the unqualified ``T`` and ``U`` refer to different types.
 *
 * This is useful for preventing ''T &&'' constructor/assignment from replacing the defaults.
 */
template <typename T, typename U>
concept non_converting = !std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>;

/**
 * @brief True if the unqualified ``T`` and ``U`` refer to different types.
 *
 * This is useful for preventing ''T &&'' constructor/assignment from replacing the defaults.
 */
template <typename T, typename U>
concept different_from = !std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>;

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
 * @brief Transform `[a, b, c] -> [f(a), f(b), f(c)]`.
 */
template <typename T, typename F>
auto map(std::vector<T> const &from, F &&func) -> std::vector<std::invoke_result_t<F &, T const &>> {

  std::vector<std::invoke_result_t<F &, T const &>> out;

  out.reserve(from.size());

  for (auto &&item : from) {
    out.emplace_back(std::invoke(func, item));
  }

  return out;
}

/**
 * @brief Transform `[a, b, c] -> [f(a), f(b), f(c)]`.
 */
template <typename T, typename F>
auto map(std::vector<T> &&from, F &&func) -> std::vector<std::invoke_result_t<F &, T>> {

  std::vector<std::invoke_result_t<F &, T>> out;

  out.reserve(from.size());

  for (auto &&item : from) {
    out.emplace_back(std::invoke(func, std::move(item)));
  }

  return out;
}

// -------------------------------- //

/**
 * @brief Returns ``ptr`` and asserts it is non-null in debug builds.
 */
template <typename T>
  requires requires (T &&ptr) {
    { ptr == nullptr } -> std::convertible_to<bool>;
  }
constexpr auto non_null(T &&ptr, std::source_location loc = std::source_location::current()) noexcept
    -> T && {
#ifndef NDEBUG
  if (ptr == nullptr) {
    // NOLINTNEXTLINE
    std::fprintf(stderr, "%s:%d: Null check failed: %s\n", loc.file_name(), loc.line(), loc.function_name());
    std::terminate();
  }
#endif
  return std::forward<T>(ptr);
}

// -------------------------------- //

/**
 * @brief Like ``std::apply`` but reverses the argument order.
 */
template <class F, class Tuple>
constexpr auto apply_to(Tuple &&tup, F &&func)
    LF_HOF_RETURNS(std::apply(std::forward<F>(func), std::forward<Tuple>(tup)))

    // -------------------------------- //

    /**
     * @brief Cast a pointer to a byte pointer.
     */
    template <typename T>
    auto byte_cast(T *ptr) LF_HOF_RETURNS(std::bit_cast<forward_cv_t<T, std::byte> *>(ptr))

} // namespace lf::impl

#endif /* DF63D333_F8C0_4BBA_97E1_32A78466B8B7 */


/**
 * @file deque.hpp
 *
 * @brief A stand-alone, production-quality implementation of the Chase-Lev lock-free
 * single-producer multiple-consumer deque.
 */

/**
 * This is a workaround for clang generating bad codegen for ``std::atomic_thread_fence``.
 */

#if defined(LF_USE_BOOST_ATOMIC) && defined(__clang__) && defined(__has_include)
  #if __has_include(<boost/atomic.hpp>)
    #include <boost/atomic.hpp>
    #define LF_ATOMIC_THREAD_FENCE_SEQ_CST boost::atomic_thread_fence(boost::memory_order_seq_cst)
  #else
    #warning "Boost.Atomic not found, falling back to std::atomic_thread_fence"
    #define LF_ATOMIC_THREAD_FENCE_SEQ_CST std::atomic_thread_fence(std::memory_order_seq_cst)
  #endif
#else
  #define LF_ATOMIC_THREAD_FENCE_SEQ_CST std::atomic_thread_fence(std::memory_order_seq_cst)
#endif

namespace lf {

inline namespace ext {

/**
 * @brief Verify a type is suitable for use with
 * [`std::atomic`](https://en.cppreference.com/w/cpp/atomic/atomic).
 *
 * This requires a `TriviallyCopyable` type satisfying both `CopyConstructible` and `CopyAssignable`.
 */
template <typename T>
concept atomicable = std::is_trivially_copyable_v<T> && //
                     std::is_copy_constructible_v<T> && //
                     std::is_move_constructible_v<T> && //
                     std::is_copy_assignable_v<T> &&    //
                     std::is_move_assignable_v<T>;      //

/**
 * @brief A concept that verifies a type is lock-free when used with `std::atomic`.
 */
template <typename T>
concept lock_free = atomicable<T> && std::atomic<T>::is_always_lock_free;

template <typename T>
concept dequeable = lock_free<T> && std::default_initializable<T>;

} // namespace ext

namespace impl {

/**
 * @brief A basic wrapper around a c-style array that provides modulo load/stores.
 *
 * This class is designed for internal use only. It provides a c-style API that is
 * used efficiently by deque for low level atomic operations.
 *
 * @tparam T The type of the elements in the array.
 */
template <dequeable T>
struct atomic_ring_buf {
  /**
   * @brief Construct a new ring buff object
   *
   * @param cap The capacity of the buffer, MUST be a power of 2.
   */
  constexpr explicit atomic_ring_buf(std::ptrdiff_t cap) : m_cap{cap}, m_mask{cap - 1} {
    LF_ASSERT(cap > 0 && std::has_single_bit(static_cast<std::size_t>(cap)));
  }
  /**
   * @brief Get the capacity of the buffer.
   */
  [[nodiscard]] constexpr auto capacity() const noexcept -> std::ptrdiff_t { return m_cap; }
  /**
   * @brief Store ``val`` at ``index % this->capacity()``.
   */
  constexpr auto store(std::ptrdiff_t index, T const &val) noexcept -> void {
    LF_ASSERT(index >= 0);
    (m_buf.get() + (index & m_mask))->store(val, std::memory_order_relaxed); // NOLINT Avoid cast.
  }
  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  [[nodiscard]] constexpr auto load(std::ptrdiff_t index) const noexcept -> T {
    LF_ASSERT(index >= 0);
    return (m_buf.get() + (index & m_mask))->load(std::memory_order_relaxed); // NOLINT Avoid cast.
  }
  /**
   * @brief Copies elements in range ``[bottom, top)`` into a new ring buffer.
   *
   * This function allocates a new buffer and returns a pointer to it.
   * The caller is responsible for deallocating the memory.
   *
   * @param bottom The bottom of the range to copy from (inclusive).
   * @param top The top of the range to copy from (exclusive).
   */
  [[nodiscard]] constexpr auto resize(std::ptrdiff_t bot, std::ptrdiff_t top) const -> atomic_ring_buf<T> * {

    auto *ptr = new atomic_ring_buf{2 * m_cap}; // NOLINT

    for (std::ptrdiff_t i = top; i != bot; ++i) {
      ptr->store(i, load(i));
    }

    return ptr;
  }

 private:
  using array_t = std::atomic<T>[]; // NOLINT

  std::ptrdiff_t m_cap;  ///< Capacity of the buffer
  std::ptrdiff_t m_mask; ///< Bit mask to perform modulo capacity operations

#ifdef __cpp_lib_smart_ptr_for_overwrite
  std::unique_ptr<array_t> m_buf = std::make_unique_for_overwrite<array_t>(static_cast<std::size_t>(m_cap));
#else
  std::unique_ptr<array_t> m_buf = std::make_unique<array_t>(static_cast<std::size_t>(m_cap));
#endif
};

} // namespace impl

inline namespace ext {

/**
 * @brief Error codes for ``deque`` 's ``steal()`` operation.
 */
enum class err : int {
  none = 0, ///< The ``steal()`` operation succeeded.
  lost,     ///< Lost the ``steal()`` race hence, the ``steal()`` operation failed.
  empty,    ///< The deque is empty and hence, the ``steal()`` operation failed.
};

/**
 * @brief The return type of a `lf::deque` `steal()` operation.
 *
 * This type is suitable for structured bindings. We return a custom type instead of a
 * `std::optional` to allow for more information to be returned as to why a steal may fail.
 */
template <typename T>
struct steal_t {
  /**
   * @brief Check if the operation succeeded.
   */
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return code == err::none; }
  /**
   * @brief Get the value like ``std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  [[nodiscard]] constexpr auto operator*() noexcept -> T & {
    LF_ASSERT(code == err::none);
    return val;
  }
  /**
   * @brief Get the value like ``std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  [[nodiscard]] constexpr auto operator*() const noexcept -> T const & {
    LF_ASSERT(code == err::none);
    return val;
  }
  /**
   * @brief Get the value ``like std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  constexpr auto operator->() noexcept -> T * {
    LF_ASSERT(code == err::none);
    return std::addressof(val);
  }
  /**
   * @brief Get the value ``like std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  constexpr auto operator->() const noexcept -> T const * {
    LF_ASSERT(code == err::none);
    return std::addressof(val);
  }

  err code; ///< The error code of the ``steal()`` operation.
  T val;    ///< The value stolen from the deque, Only valid if ``code == err::stolen``.
};

/**
 * @brief An unbounded lock-free single-producer multiple-consumer work-stealing deque.
 *
 * \rst
 *
 * Implements the "Chase-Lev" deque described in the papers, `"Dynamic Circular Work-Stealing deque"
 * <https://doi.org/10.1145/1073970.1073974>`_ and `"Correct and Efficient Work-Stealing for Weak
 * Memory Models" <https://doi.org/10.1145/2442516.2442524>`_.
 *
 * Only the deque owner can perform ``pop()`` and ``push()`` operations where the deque behaves
 * like a LIFO stack. Others can (only) ``steal()`` data from the deque, they see a FIFO deque.
 * All threads must have finished using the deque before it is destructed.
 *
 *
 * Example:
 *
 * .. include:: ../../../test/source/schedule/deque.cpp
 *    :code:
 *    :start-after: // !BEGIN-EXAMPLE
 *    :end-before: // !END-EXAMPLE
 *
 * \endrst
 *
 * @tparam T The type of the elements in the deque.
 * @tparam Optional The type returned by ``pop()``.
 */
template <dequeable T>
class deque : impl::immovable<deque<T>> {

  static constexpr std::ptrdiff_t k_default_capacity = 1024;
  static constexpr std::size_t k_garbage_reserve = 64;

 public:
  /**
   * @brief The type of the elements in the deque.
   */
  using value_type = T;
  /**
   * @brief Construct a new empty deque object.
   */
  constexpr deque() : deque(k_default_capacity) {}
  /**
   * @brief Construct a new empty deque object.
   *
   * @param cap The capacity of the deque (must be a power of 2).
   */
  constexpr explicit deque(std::ptrdiff_t cap);
  /**
   * @brief Get the number of elements in the deque.
   */
  [[nodiscard]] constexpr auto size() const noexcept -> std::size_t;
  /**
   * @brief Get the number of elements in the deque as a signed integer.
   */
  [[nodiscard]] constexpr auto ssize() const noexcept -> ptrdiff_t;
  /**
   * @brief Get the capacity of the deque.
   */
  [[nodiscard]] constexpr auto capacity() const noexcept -> ptrdiff_t;
  /**
   * @brief Check if the deque is empty.
   */
  [[nodiscard]] constexpr auto empty() const noexcept -> bool;
  /**
   * @brief Push an item into the deque.
   *
   * Only the owner thread can insert an item into the deque.
   * This operation can trigger the deque to resize if more space is required.
   *
   * @param val Value to add to the deque.
   */
  LF_FORCEINLINE constexpr void push(T const &val) noexcept;
  /**
   * @brief Pop an item from the deque.
   *
   * Only the owner thread can pop out an item from the deque. If the buffer is empty calls `when_empty` and
   * returns the result. By default, `when_empty` is a no-op that returns a null `std::optional<T>`.
   */
  template <std::invocable F = impl::return_nullopt<T>>
    requires std::convertible_to<T, std::invoke_result_t<F>>
  LF_FORCEINLINE constexpr auto pop(F &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<F>)
      -> std::invoke_result_t<F>;

  /**
   * @brief Steal an item from the deque.
   *
   * Any threads can try to steal an item from the deque. This operation can fail if the deque is
   * empty or if another thread simultaneously stole an item from the deque.
   */
  constexpr auto steal() noexcept -> steal_t<T>;
  /**
   * @brief Destroy the deque object.
   *
   * All threads must have finished using the deque before it is destructed.
   */
  constexpr ~deque() noexcept;

 private:
  alignas(impl::k_cache_line) std::atomic<std::ptrdiff_t> m_top;
  alignas(impl::k_cache_line) std::atomic<std::ptrdiff_t> m_bottom;
  alignas(impl::k_cache_line) std::atomic<impl::atomic_ring_buf<T> *> m_buf;
  std::vector<std::unique_ptr<impl::atomic_ring_buf<T>>> m_garbage;

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <dequeable T>
constexpr deque<T>::deque(std::ptrdiff_t cap)
    : m_top(0),
      m_bottom(0),
      m_buf(new impl::atomic_ring_buf<T>{cap}) {
  m_garbage.reserve(k_garbage_reserve);
}

template <dequeable T>
constexpr auto deque<T>::size() const noexcept -> std::size_t {
  return static_cast<std::size_t>(ssize());
}

template <dequeable T>
constexpr auto deque<T>::ssize() const noexcept -> std::ptrdiff_t {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return std::max(bottom - top, ptrdiff_t{0});
}

template <dequeable T>
constexpr auto deque<T>::capacity() const noexcept -> ptrdiff_t {
  return m_buf.load(relaxed)->capacity();
}

template <dequeable T>
constexpr auto deque<T>::empty() const noexcept -> bool {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return top >= bottom;
}

template <dequeable T>
LF_FORCEINLINE constexpr auto deque<T>::push(T const &val) noexcept -> void {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(acquire);
  impl::atomic_ring_buf<T> *buf = m_buf.load(relaxed);

  if (buf->capacity() < (bottom - top) + 1) {
    // Deque is full, build a new one, this can never throw as we reserve 64 slots.
    m_garbage.emplace_back(std::exchange(buf, buf->resize(bottom, top)));
    m_buf.store(buf, relaxed);
  }

  // Construct new object, this does not have to be atomic as no one can steal this item until
  // after we store the new value of bottom, ordering is maintained by surrounding atomics.
  buf->store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);
}

template <dequeable T>
template <std::invocable F>
  requires std::convertible_to<T, std::invoke_result_t<F>>
LF_FORCEINLINE constexpr auto deque<T>::pop(F &&when_empty) noexcept(std::is_nothrow_invocable_v<F>)
    -> std::invoke_result_t<F> {

  std::ptrdiff_t const bottom = m_bottom.load(relaxed) - 1; //
  impl::atomic_ring_buf<T> *buf = m_buf.load(relaxed);      //
  m_bottom.store(bottom, relaxed);                          // Stealers can no longer steal.

  LF_ATOMIC_THREAD_FENCE_SEQ_CST;

  std::ptrdiff_t top = m_top.load(relaxed);

  if (top <= bottom) {
    // Non-empty deque
    if (top == bottom) {
      // The last item could get stolen, by a stealer that loaded bottom before our write above.
      if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
        // Failed race, thief got the last item.
        m_bottom.store(bottom + 1, relaxed);
        return std::invoke(std::forward<F>(when_empty));
      }
      m_bottom.store(bottom + 1, relaxed);
    }
    // Can delay load until after acquiring slot as only this thread can push(),
    // This load is not required to be atomic as we are the exclusive writer.
    return buf->load(bottom);
  }
  m_bottom.store(bottom + 1, relaxed);
  return std::invoke(std::forward<F>(when_empty));
}

template <dequeable T>
constexpr auto deque<T>::steal() noexcept -> steal_t<T> {
  std::ptrdiff_t top = m_top.load(acquire);
  LF_ATOMIC_THREAD_FENCE_SEQ_CST;
  std::ptrdiff_t const bottom = m_bottom.load(acquire);

  if (top < bottom) {
    // Must load *before* acquiring the slot as slot may be overwritten immediately after
    // acquiring. This load is NOT required to be atomic even-though it may race with an overwrite
    // as we only return the value if we win the race below guaranteeing we had no race during our
    // read. If we loose the race then 'x' could be corrupt due to read-during-write race but as T
    // is trivially destructible this does not matter.
    T tmp = m_buf.load(consume)->load(top);

    static_assert(std::is_trivially_destructible_v<T>, "concept 'atomicable' should guarantee this already");

    if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
      return {.code = err::lost, .val = {}};
    }
    return {.code = err::none, .val = tmp};
  }
  return {.code = err::empty, .val = {}};
}

template <dequeable T>
constexpr deque<T>::~deque() noexcept {
  delete m_buf.load(); // NOLINT
}

} // namespace ext

} // namespace lf

#undef LF_ATOMIC_THREAD_FENCE_SEQ_CST

#endif /* C9703881_3D9C_41A5_A7A2_44615C4CFA6A */
#ifndef ACB944D8_08B6_4600_9302_602E847753FD
#define ACB944D8_08B6_4600_9302_602E847753FD

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits>
#include <version>
#ifndef DD6F6C5C_C146_4C02_99B9_7D2D132C0844
#define DD6F6C5C_C146_4C02_99B9_7D2D132C0844

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <coroutine>
#include <exception>
#include <semaphore>
#include <type_traits>
#include <utility>
#ifndef B4EE570B_F5CF_42CB_9AF3_7376F45FDACC
#define B4EE570B_F5CF_42CB_9AF3_7376F45FDACC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>



/**
 * @file defer.hpp
 *
 * @brief A Golang-like defer implemented via destructor calls.
 */

namespace lf {

inline namespace core {

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
 *    defer _ = [&ptr] () noexcept {
 *      c_api_clean_up(ptr);
 *    };
 *
 *    // Code that may throw
 *
 * \endrst
 *
 * You can also use the ``LF_DEFER`` macro to create an automatically named defer object.
 *
 */
template <class F>
  requires std::is_nothrow_invocable_v<F>
class [[nodiscard("Defer will execute unless bound to a name!")]] defer : impl::immovable<defer<F>> {
 public:
  /**
   * @brief Construct a new Defer object.
   *
   * @param f Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(F &&f) noexcept(std::is_nothrow_constructible_v<F, F &&>) : m_f(std::forward<F>(f)) {}

  /**
   * @brief Calls the invocable.
   */
  LF_FORCEINLINE constexpr ~defer() noexcept { std::invoke(std::forward<F>(m_f)); }

 private:
  [[no_unique_address]] F m_f;
};

/**
 * @brief A macro to create an automatically named defer object.
 */
#define LF_DEFER ::lf::defer LF_CONCAT_OUTER(at_exit_, __LINE__) = [&]() noexcept

} // namespace core

} // namespace lf

#endif /* B4EE570B_F5CF_42CB_9AF3_7376F45FDACC */

#ifndef F51F8998_9E69_458E_95E1_8592A49FA76C
#define F51F8998_9E69_458E_95E1_8592A49FA76C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <new>


/**
 * @file manual_lifetime.hpp
 *
 * @brief A utility class for explicitly managing the lifetime of an object.
 */

namespace lf::impl {

// TODO: we could make manual_lifetime<T> empty if T is empty?

/**
 * @brief Provides storage for a single object of type ``T``.
 *
 * Every instance of manual_lifetime is trivially constructible/destructible.
 */
template <typename T>
class manual_lifetime : immovable<manual_lifetime<T>> {
 public:
  /**
   * @brief Start lifetime of object.
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  auto construct(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> T * {
    return ::new (static_cast<void *>(m_buf.data())) T(std::forward<Args>(args)...);
  }

  /**
   * @brief Start lifetime of object at assignment.
   */
  template <typename U>
    requires std::constructible_from<T, U>
  void operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) {
    this->construct(std::forward<U>(expr));
  }

  /**
   * @brief Destroy the contained object, must have been constructed first.
   *
   * A noop if ``T`` is trivially destructible.
   */
  void destroy() noexcept(std::is_nothrow_destructible_v<T>)
    requires std::is_destructible_v<T>
  {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      std::destroy_at(data());
    }
  }

  /**
   * @brief Get a pointer to the contained object, must have been constructed first.
   */
  [[nodiscard]] auto data() noexcept -> T * { return std::launder(reinterpret_cast<T *>(m_buf.data())); }

  /**
   * @brief Get a pointer to the contained object, must have been constructed first.
   */
  [[nodiscard]] auto data() const noexcept -> T * {
    return std::launder(reinterpret_cast<T const *>(m_buf.data()));
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() noexcept -> T * { return data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() const noexcept -> T const * { return data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() & noexcept -> T & { return *data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const & noexcept -> T const & { return *data(); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() && noexcept -> T && { return std::move(*data()); }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T const && { return std::move(*data()); }

 private:
  [[no_unique_address]] alignas(T) std::array<std::byte, sizeof(T)> m_buf;
};

} // namespace lf::impl

#endif /* F51F8998_9E69_458E_95E1_8592A49FA76C */
#ifndef F7577AB3_0439_404D_9D98_072AB84FBCD0
#define F7577AB3_0439_404D_9D98_072AB84FBCD0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdlib>

#include <memory>
#include <type_traits>
#include <utility>



/**
 * @file stack.hpp
 *
 * @brief Implementation of libfork's geometric segmented stacks.
 */

#ifndef LF_FIBRE_INIT_SIZE
  /**
   * @brief The initial size for a stack (in bytes).
   *
   * All stacklets will be round up to a multiple of the page size.
   */
  #define LF_FIBRE_INIT_SIZE 1
#endif

static_assert(LF_FIBRE_INIT_SIZE > 0, "Stacks must have a positive size");

namespace lf::impl {

/**
 * @brief Round size close to a multiple of the page_size.
 */
inline constexpr auto round_up_to_page_size(std::size_t size) noexcept -> std::size_t {

  // Want calculate req such that:

  // req + malloc_block_est is a multiple of the page size.
  // req > size + stacklet_size

  std::size_t constexpr page_size = 4096;                           // 4 KiB on most systems.
  std::size_t constexpr malloc_meta_data_size = 6 * sizeof(void *); // An (over)estimate.

  static_assert(std::has_single_bit(page_size));

  std::size_t minimum = size + malloc_meta_data_size;
  std::size_t rounded = (minimum + page_size - 1) & ~(page_size - 1);
  std::size_t request = rounded - malloc_meta_data_size;

  LF_ASSERT(minimum <= rounded);
  LF_ASSERT(rounded % page_size == 0);
  LF_ASSERT(request >= size);

  return request;
}

/**
 * @brief A stack is a user-space (geometric) segmented program stack.
 *
 * A stack stores the execution of a DAG from root (which may be a stolen task or true root) to suspend
 * point. A stack is composed of stacklets, each stacklet is a contiguous region of stack space stored in a
 * double-linked list. A stack tracks the top stacklet, the top stacklet contains the last allocation or the
 * stack is empty. The top stacklet may have zero or one cached stacklets "ahead" of it.
 */
class stack {

 public:
  /**
   * @brief A stacklet is a stack fragment that contains a segment of the stack.
   *
   * A chain of stacklets looks like `R <- F1 <- F2 <- F3 <- ... <- Fn` where `R` is the root stacklet.
   *
   * A stacklet is allocated as a contiguous chunk of memory, the first bytes of the chunk contain the
   * stacklet object. Semantically, a stacklet is a dynamically sized object.
   *
   * Each stacklet also contains an exception pointer and atomic flag which stores exceptions thrown by
   * children.
   */
  class alignas(impl::k_new_align) stacklet : impl::immovable<stacklet> {

    friend class stack;

    /**
     * @brief Capacity of the current stacklet's stack.
     */
    [[nodiscard]] auto capacity() const noexcept -> std::size_t {
      LF_ASSERT(m_hi - m_lo >= 0);
      return m_hi - m_lo;
    }

    /**
     * @brief Unused space on the current stacklet's stack.
     */
    [[nodiscard]] auto unused() const noexcept -> std::size_t {
      LF_ASSERT(m_hi - m_sp >= 0);
      return m_hi - m_sp;
    }

    /**
     * @brief Check if stacklet's stack is empty.
     */
    [[nodiscard]] auto empty() const noexcept -> bool { return m_sp == m_lo; }

    /**
     * @brief Check is this stacklet is the top of a stack.
     */
    [[nodiscard]] auto is_top() const noexcept -> bool {
      if (m_next != nullptr) {
        // Accept a single cached stacklet above the top.
        return m_next->empty() && m_next->m_next == nullptr;
      }
      return true;
    }

    /**
     * @brief Set the next stacklet in the chain to 'new_next'.
     *
     * This requires that this is the top stacklet. If there is a cached stacklet ahead of the top stacklet
     * then it will be freed before being replaced with 'new_next'.
     */
    void set_next(stacklet *new_next) noexcept {
      LF_ASSERT(is_top());
      std::free(std::exchange(m_next, new_next)); // NOLINT
    }

    /**
     * @brief Allocate a new stacklet with a stack of size of at least`size` and attach it to the given
     * stacklet chain.
     *
     * Requires that `prev` must be the top stacklet in a chain or `nullptr`. This will round size up to
     */
    [[nodiscard]] LF_NOINLINE static auto next_stacklet(std::size_t size, stacklet *prev) -> stacklet * {

      LF_LOG("allocating a new stacklet");

      LF_ASSERT(prev == nullptr || prev->is_top());

      std::size_t request = impl::round_up_to_page_size(size + sizeof(stacklet));

      LF_ASSERT(request >= sizeof(stacklet) + size);

      stacklet *next = static_cast<stacklet *>(std::malloc(request)); // NOLINT

      if (next == nullptr) {
        LF_THROW(std::bad_alloc());
      }

      if (prev != nullptr) {
        // Set next tidies up other next.
        prev->set_next(next);
      }

      next->m_lo = impl::byte_cast(next) + sizeof(stacklet);
      next->m_sp = next->m_lo;
      next->m_hi = impl::byte_cast(next) + request;

      next->m_prev = prev;
      next->m_next = nullptr;

      return next;
    }

    /**
     * @brief Allocate an initial stacklet.
     */
    [[nodiscard]] static auto next_stacklet() -> stacklet * {
      return stacklet::next_stacklet(LF_FIBRE_INIT_SIZE, nullptr);
    }

    std::byte *m_lo;  ///< This stacklet's stack.
    std::byte *m_sp;  ///< The current position of the stack pointer in the stack.
    std::byte *m_hi;  ///< The one-past-the-end address of the stack.
    stacklet *m_prev; ///< Doubly linked list (past).
    stacklet *m_next; ///< Doubly linked list (future).
  };

  // Keep stack aligned.
  static_assert(sizeof(stacklet) >= impl::k_new_align);
  static_assert(sizeof(stacklet) % impl::k_new_align == 0);
  // Stacklet is implicit lifetime type
  static_assert(std::is_trivially_default_constructible_v<stacklet>);
  static_assert(std::is_trivially_destructible_v<stacklet>);

  /**
   * @brief Constructs a stack with a small empty stack.
   */
  stack() : m_fib(stacklet::next_stacklet()) { LF_LOG("Constructing a stack"); }

  /**
   * @brief Construct a new stack object taking ownership of the stack that `frag` is a top-of.
   */
  explicit stack(stacklet *frag) noexcept : m_fib(frag) {
    LF_LOG("Constructing stack from stacklet");
    LF_ASSERT(frag && frag->is_top());
  }

  stack(std::nullptr_t) = delete;

  stack(stack const &) = delete;

  auto operator=(stack const &) -> stack & = delete;

  /**
   * @brief Move-construct from `other` leaves `other` in the empty/default state.
   */
  stack(stack &&other) : stack() { swap(*this, other); }

  /**
   * @brief Swap this and `other`.
   */
  auto operator=(stack &&other) noexcept -> stack & {
    swap(*this, other);
    return *this;
  }

  /**
   * @brief Swap `lhs` with `rhs.
   */
  inline friend void swap(stack &lhs, stack &rhs) noexcept {
    using std::swap;
    swap(lhs.m_fib, rhs.m_fib);
  }

  ~stack() noexcept {
    LF_ASSERT(m_fib);
    LF_ASSERT(!m_fib->m_prev); // Should only be destructed at the root.
    m_fib->set_next(nullptr);  // Free a cached stacklet.
    std::free(m_fib);          // NOLINT
  }

  [[nodiscard]] auto empty() -> bool {
    LF_ASSERT(m_fib && m_fib->is_top());
    return m_fib->empty();
  }

  /**
   * @brief Release the underlying storage of the current stack and re-initialize this one.
   *
   * A new stack can be constructed from the stacklet to continue the released stack.
   */
  [[nodiscard]] auto release() -> stacklet * {
    LF_LOG("Releasing stack");
    LF_ASSERT(m_fib);
    return std::exchange(m_fib, stacklet::next_stacklet());
  }

  /**
   * @brief Allocate `count` bytes of memory on a stacklet in the bundle.
   *
   * The memory will be aligned to a multiple of `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
   *
   * Deallocate the memory with `deallocate` in a FILO manor.
   */
  [[nodiscard]] LF_FORCEINLINE auto allocate(std::size_t size) -> void * {
    //
    LF_ASSERT(m_fib && m_fib->is_top());

    // Round up to the next multiple of the alignment.
    std::size_t ext_size = (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);

    if (m_fib->unused() < ext_size) {
      if (m_fib->m_next != nullptr && m_fib->m_next->capacity() >= ext_size) {
        m_fib = m_fib->m_next;
      } else {
        m_fib = stacklet::next_stacklet(std::max(2 * m_fib->capacity(), ext_size), m_fib);
      }
    }

    LF_ASSERT(m_fib && m_fib->is_top());

    LF_LOG("Allocating {} bytes {}-{}", size, (void *)m_fib->m_sp, (void *)(m_fib->m_sp + ext_size));

    return std::exchange(m_fib->m_sp, m_fib->m_sp + ext_size);
  }

  /**
   * @brief Deallocate `count` bytes of memory from the current stack.
   *
   * This must be called in FILO order with `allocate`.
   */
  LF_FORCEINLINE void deallocate(void *ptr) noexcept {

    LF_ASSERT(m_fib && m_fib->is_top());

    LF_LOG("Deallocating {}", ptr);

    m_fib->m_sp = static_cast<std::byte *>(ptr);

    if (m_fib->empty()) {

      if (m_fib->m_prev != nullptr) {
        // Always free a second order cached stacklet if it exists.
        m_fib->set_next(nullptr);
        // Move to prev stacklet.
        m_fib = m_fib->m_prev;
      }

      LF_ASSERT(m_fib);

      // Guard against over-caching.
      if (m_fib->m_next != nullptr) {
        if (m_fib->m_next->capacity() > 8 * m_fib->capacity()) {
          // Free oversized stacklet.
          m_fib->set_next(nullptr);
        }
      }
    }

    LF_ASSERT(m_fib && m_fib->is_top());
  }

  /**
   * @brief Get the stacklet that the last allocation was on, this is non-null.
   */
  [[nodiscard]] auto top() noexcept -> stacklet * {
    LF_ASSERT(m_fib && m_fib->is_top());
    return non_null(m_fib);
  }

 private:
  stacklet *m_fib; ///< The allocation stacklet.
};

} // namespace lf::impl

#endif /* F7577AB3_0439_404D_9D98_072AB84FBCD0 */


/**
 * @file frame.hpp
 *
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */

namespace lf::impl {

/**
 * @brief A small structure that acts a bit like a root task's parent.
 */
struct root_notify {
  std::exception_ptr m_eptr;    ///< Maybe an exception pointer.
  std::binary_semaphore sem{0}; ///< Notified when root task completes
};

/**
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */
class frame {

#if LF_COMPILER_EXCEPTIONS
  manual_lifetime<std::exception_ptr> m_eptr; ///< Maybe an exception pointer.
#endif

#ifndef LF_COROUTINE_OFFSET
  std::coroutine_handle<> m_this_coro; ///< Handle to this coroutine.
#endif

  stack::stacklet *m_stacklet; ///< Needs to be in promise in case allocation elided (as does m_parent).

  union {
    frame *m_parent;       ///< Non-root tasks store a pointer to their parent.
    root_notify *m_notify; ///< Root tasks store a pointer to a notifier
  };

  std::atomic_uint16_t m_join = k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;               ///< Number of times this frame has been stolen.

#if LF_COMPILER_EXCEPTIONS
  bool m_except = false; ///< Flag to indicate if an exception has been set.
#endif

  /**
   * @brief Cold path in `rethrow_if_exception` in its own non-inline function.
   */
  LF_NOINLINE void rethrow() {
#if LF_COMPILER_EXCEPTIONS

    LF_ASSERT(*m_eptr != nullptr);

    LF_DEFER {
      LF_ASSERT(*m_eptr == nullptr);
      m_eptr.destroy();
      m_except = false;
    };

    std::rethrow_exception(std::exchange(*m_eptr, nullptr));
#endif
  }

 public:
  /**
   * @brief Construct a frame block.
   *
   * Non-root tasks will need to call ``set_parent(...)``.
   */
#ifndef LF_COROUTINE_OFFSET
  frame(std::coroutine_handle<> coro, stack::stacklet *stacklet) noexcept
      : m_this_coro{coro},
        m_stacklet(non_null(stacklet)) {
    LF_ASSERT(coro);
  }
#else
  frame(std::coroutine_handle<>, stack::stacklet *stacklet) noexcept : m_stacklet(non_null(stacklet)) {}
#endif

  /**
   * @brief Set the pointer to the parent frame.
   */
  void set_parent(frame *parent) noexcept { m_parent = non_null(parent); }

  /**
   * @brief Set a root tasks parent.
   */
  void set_root_notify(root_notify *notify) noexcept { m_notify = non_null(notify); }

  /**
   * @brief Set the stacklet object to point at a new stacklet.
   *
   * Returns the previous stacklet.
   */
  auto reset_stacklet(stack::stacklet *stacklet) noexcept -> stack::stacklet * {
    return std::exchange(m_stacklet, non_null(stacklet));
  }

  /**
   * @brief Get a pointer to the parent frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame * { return m_parent; }

  /**
   * @brief Get a pointer to the notifier for this root frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto notifier() const noexcept -> root_notify * { return m_notify; }

  /**
   * @brief Get a pointer to the top of the top of the stack-stack this frame was allocated on.
   */
  [[nodiscard]] auto stacklet() const noexcept -> stack::stacklet * { return non_null(m_stacklet); }

  /**
   * @brief Get the coroutine handle for this frames coroutine.
   */
  [[nodiscard]] auto self() noexcept -> std::coroutine_handle<> {
#ifndef LF_COROUTINE_OFFSET
    return m_this_coro;
#else
    return std::coroutine_handle<>::from_address(byte_cast(this) - LF_COROUTINE_OFFSET);
#endif
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] auto load_joins(std::memory_order order) const noexcept -> std::uint16_t {
    return m_join.load(order);
  }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  auto fetch_sub_joins(std::uint16_t val, std::memory_order order) noexcept -> std::uint16_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] auto load_steals() const noexcept -> std::uint16_t { return m_steal; }

  /**
   * @brief Increase the steal counter by one and return the previous value.
   */
  auto fetch_add_steal() noexcept -> std::uint16_t { return m_steal++; }

  /**
   * @brief Reset the join and steal counters, must be outside a fork-join region.
   */
  void reset() noexcept {

    m_steal = 0;

    static_assert(std::is_trivially_destructible_v<decltype(m_join)>);
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    std::construct_at(&m_join, k_u16_max);
  }

  /**
   * @brief Capture the exception currently being thrown.
   *
   * Safe to call concurrently, first exception is saved.
   */
  void capture_exception() noexcept {
#if LF_COMPILER_EXCEPTIONS
    if (!std::atomic_ref{m_except}.exchange(true, std::memory_order_acq_rel)) {
      m_eptr.construct(std::current_exception());
    }
#endif
  }

  /**
   * @brief If this contains an exception then it will be rethrown, reset this object to the OK state.
   *
   * This can __only__ be called when the caller has exclusive ownership over this object.
   */
  LF_FORCEINLINE void rethrow_if_exception() {
#if LF_COMPILER_EXCEPTIONS
    if (m_except) {
      rethrow();
    }
#endif
  }

  /**
   * @brief Check if this contains an exception.
   *
   * This can __only__ be called when the caller has exclusive ownership over this object.
   */
  [[nodiscard]] auto has_exception() const noexcept -> bool {
#if LF_COMPILER_EXCEPTIONS
    return m_except;
#else
    return false;
#endif
  }
};

static_assert(std::is_standard_layout_v<frame>);

} // namespace lf::impl

#endif /* DD6F6C5C_C146_4C02_99B9_7D2D132C0844 */


/**
 * @file handles.hpp
 *
 * @brief Definitions of `libfork`'s handle types.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A type safe wrapper around a handle to a coroutine that is at a submission point.
 *
 * Instances of this type (wrapped in an `lf::intrusive_list`s node) will be passed to a worker's context.
 *
 * \rst
 *
 * .. note::
 *
 *    A pointer to an ``submit_h`` should only ever passed to ``resume()``.
 *
 * \endrst
 */
class submit_t : impl::frame {};

static_assert(std::is_standard_layout_v<submit_t>);

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_base_of_v<impl::frame, submit_t>);
#endif

/**
 * @brief An alias for a pointer to a `submit_t`.
 */
using submit_handle = submit_t *;

/**
 * @brief A type safe wrapper around a handle to a stealable coroutine.
 *
 * Instances of this type will be passed to a worker's context.
 *
 * \rst
 *
 * .. note::
 *
 *    A pointer to an ``task_h`` should only ever passed to ``resume()``.
 *
 * \endrst
 */
class task_t : impl::frame {};

static_assert(std::is_standard_layout_v<task_t>);

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_base_of_v<impl::frame, task_t>);
#endif

/**
 * @brief An alias for a pointer to a `task_t`.
 */
using task_handle = task_t *;

} // namespace ext

} // namespace lf

#endif /* ACB944D8_08B6_4600_9302_602E847753FD */
#ifndef BC7496D2_E762_43A4_92A3_F2AD10690569
#define BC7496D2_E762_43A4_92A3_F2AD10690569

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
 * @file list.hpp
 *
 * @brief Lock-free intrusive list for use when submitting tasks.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A multi-producer, single-consumer intrusive list.
 *
 * This implementation is lock-free, allocates no memory and is optimized for weak memory models.
 */
template <typename T>
class intrusive_list : impl::immovable<intrusive_list<T>> {
 public:
  /**
   * @brief An intruded node in the list.
   */
  class node : impl::immovable<node> {
   public:
    /**
     * @brief Construct a node storing a copy of `data`.
     */
    explicit constexpr node(T const &data) noexcept(std::is_nothrow_copy_constructible_v<T>) : m_data(data) {}

    /**
     * @brief Access the value stored in a node of the list.
     */
    friend constexpr auto unwrap(node *ptr) noexcept -> T & { return non_null(ptr)->m_data; }

    /**
     * @brief Call `func` on each unwrapped node linked in the list.
     *
     * This is a noop if `root` is `nullptr`.
     */
    template <std::invocable<T &> F>
    friend constexpr void for_each_elem(node *root, F &&func) noexcept(std::is_nothrow_invocable_v<F, T &>) {
      while (root) {
        // Have to be very careful here, we can't deference `root` after
        // we've called `func` as `func` could destroy the node so, we have
        // to cache the next pointer before the function call.
        auto next = root->m_next;
        std::invoke(func, root->m_data);
        root = next;
      }
    }

   private:
    friend class intrusive_list;

    [[no_unique_address]] T m_data;
    node *m_next = nullptr;
  };

  /**
   * @brief Push a new node, this can be called concurrently from any number of threads.
   *
   * `new_node` should be an unlinked node e.g. not part of a list.
   */
  constexpr void push(node *new_node) noexcept {

    LF_ASSERT(new_node && new_node->m_next == nullptr);

    node *stale_head = m_head.load(std::memory_order_relaxed);

    for (;;) {
      non_null(new_node)->m_next = stale_head;

      if (m_head.compare_exchange_weak(stale_head, new_node, std::memory_order_release)) {
        return;
      }
    }
  }

  /**
   * @brief Pop all the nodes from the list and return a pointer to the root (`nullptr` if empty).
   *
   * Only the owner (thread) of the list can call this function, this will reverse the direction of the list
   * such that `for_each_elem` will operate if FIFO order.
   */
  constexpr auto try_pop_all() noexcept -> node * {

    node *last = m_head.exchange(nullptr, std::memory_order_consume);
    node *first = nullptr;

    while (last) {
      node *tmp = last;
      last = last->m_next;
      tmp->m_next = first;
      first = tmp;
    }

    return first;
  }

 private:
  std::atomic<node *> m_head = nullptr;
};

/**
 * @brief A type alias for the node type of an intrusive list.
 */
template <typename T>
using intruded_list = typename intrusive_list<T>::node *;

} // namespace ext

} // namespace lf

#endif /* BC7496D2_E762_43A4_92A3_F2AD10690569 */



/**
 * @file context.hpp
 *
 * @brief Provides the hierarchy of worker thread contexts.
 */

namespace lf {

// ------------------ Context ------------------- //

inline namespace ext {

class context; // Semi-User facing, (for submitting tasks).

class worker_context; // API for worker threads.

} // namespace ext

namespace impl {

class full_context; // Internal API

struct switch_awaitable; // Forwadr decl for friend.

} // namespace impl

inline namespace ext {

/**
 * @brief A type-erased function object that takes no arguments.
 */
#ifdef __cpp_lib_move_only_function
using nullary_function_t = std::move_only_function<void()>;
#else
using nullary_function_t = std::function<void()>;
#endif

/**
 * @brief Core-visible context for users to query and submit tasks to.
 */
class context : impl::immovable<context> {
 public:
  /**
   * @brief Construct a context for a worker thread.
   *
   * Notify is a function that may be called concurrently by other workers to signal to the worker
   * owning this context that a task has been submitted to a private queue.
   */
  explicit context(nullary_function_t notify) noexcept : m_notify(std::move(notify)) { LF_ASSERT(m_notify); }

 private:
  friend class ext::worker_context;
  friend class impl::full_context;
  friend struct impl::switch_awaitable;

  /**
   * @brief Submit pending/suspended tasks to the context.
   *
   * This is for use by the implementer of the scheduler, this will trigger the notification function.
   */
  void submit(intruded_list<submit_handle> jobs) {
    m_submit.push(non_null(jobs));
    m_notify();
  }

  deque<task_handle> m_tasks;             ///< All non-null.
  intrusive_list<submit_handle> m_submit; ///< All non-null.

  nullary_function_t m_notify; ///< The user supplied notification function.
};

/**
 * @brief Context for (user) schedulers to interact with.
 *
 * Additionally exposes submit and pop functions.
 */
class worker_context : public context {
 public:
  using context::context;

  using context::submit; ///< Submit an external task.

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   *
   * If there are no submitted tasks, then returned pointer will be null.
   */
  [[nodiscard]] auto try_pop_all() noexcept -> intruded_list<submit_handle> { return m_submit.try_pop_all(); }

  /**
   * @brief Attempt a steal operation from this contexts task deque.
   */
  [[nodiscard]] auto try_steal() noexcept -> steal_t<task_handle> { return m_tasks.steal(); }
};

} // namespace ext

namespace impl {

/**
 * @brief Context for internal use, contains full-API for push/pop.
 */
class full_context : public worker_context {
 public:
  using worker_context::worker_context;

  void push(task_handle task) noexcept { m_tasks.push(non_null(task)); }

  [[nodiscard]] auto pop() noexcept -> task_handle {
    return m_tasks.pop([]() -> task_handle {
      return nullptr;
    });
  }

  [[nodiscard]] auto empty() const noexcept -> bool { return m_tasks.empty(); }
};

} // namespace impl

} // namespace lf

#endif /* D66BBECE_E467_4EB6_B74A_AAA2E7256E02 */



/**
 * @file tls.hpp
 *
 * @brief The thread local variables used by libfork.
 */

namespace lf {

namespace impl::tls {

/**
 * @brief Set when `impl::tls::thread_stack` is alive.
 */
constinit inline thread_local bool has_stack = false;
/**
 * @brief A workers stack.
 *
 * This is wrapped in an `manual_lifetime` to make it trivially destructible/constructible such that it
 * requires no construction checks to access.
 */
constinit inline thread_local manual_lifetime<stack> thread_stack = {};
/**
 * @brief Set when `impl::tls::thread_stack` is alive.
 */
constinit inline thread_local bool has_context = false;
/**
 * @brief A workers context.
 *
 * This is wrapped in an `manual_lifetime` to make it trivially destructible/constructible such that it
 * requires no construction checks to access.
 */
constinit inline thread_local manual_lifetime<full_context> thread_context = {};

/**
 * @brief Checked access to a workers stack.
 */
[[nodiscard]] inline auto stack() -> stack * {
  LF_ASSERT(has_stack);
  return thread_stack.data();
}

/**
 * @brief Checked access to a workers context.
 */
[[nodiscard]] inline auto context() -> full_context * {
  LF_ASSERT(has_context);
  return thread_context.data();
}

} // namespace impl::tls

inline namespace ext {

/**
 * @brief Initialize thread-local variables before a worker can resume submitted tasks.
 *
 * \rst
 *
 * .. warning::
 *    These should be cleaned up with ``worker_finalize(...)``.
 *
 * \endrst
 */
[[nodiscard]] inline auto worker_init(nullary_function_t notify) -> worker_context * {

  LF_LOG("Initializing worker");

  if (impl::tls::has_context && impl::tls::has_stack) {
    LF_THROW(std::runtime_error("Worker already initialized"));
  }

  worker_context *context = impl::tls::thread_context.construct(std::move(notify));

  // clang-format off

  LF_TRY {
    impl::tls::thread_stack.construct();
  } LF_CATCH_ALL {
    impl::tls::thread_context.destroy();
  }

  impl::tls::has_stack = true;
  impl::tls::has_context = true;

  // clang-format on

  return context;
}

/**
 * @brief Clean-up thread-local variable before destructing a worker's context.
 *
 * \rst
 *
 * .. warning::
 *    These must be initialized with ``worker_init(...)``.
 *
 * \endrst
 */
inline void finalize(worker_context *worker) {

  LF_LOG("Finalizing worker");

  if (worker != impl::tls::thread_context.data()) {
    LF_THROW(std::runtime_error("Finalize called on wrong thread"));
  }

  if (!impl::tls::has_context || !impl::tls::has_stack) {
    LF_THROW(std::runtime_error("Finalize called before initialization or after finalization"));
  }

  impl::tls::thread_context.destroy();
  impl::tls::thread_stack.destroy();

  impl::tls::has_stack = false;
  impl::tls::has_context = false;
}

} // namespace ext

} // namespace lf

#endif /* CF97E524_27A6_4CD9_8967_39F1B1BE97B6 */



/**
 * @file co_alloc.hpp
 *
 * @brief Expert-only utilities to interact with a coroutines stack.
 */

namespace lf {

inline namespace core {

/**
 * @brief Check is a type is suitable for allocation on libfork's stacks.
 *
 * This requires the type to be `std::default_initializable<T>` and have non-new-extended alignment.
 */
template <typename T>
concept co_allocable = std::default_initializable<T> && alignof(T) <= impl::k_new_align;

} // namespace core

namespace impl {

/**
 * @brief An awaitable (in the context of an ``lf::task``) which triggers stack allocation.
 */
template <co_allocable T, std::size_t Extent>
struct [[nodiscard("This object should be co_awaited")]] co_new_t {
  static constexpr std::size_t count = Extent; ///< The number of elements to allocate.
};

/**
 * @brief An awaitable (in the context of an ``lf::task``) which triggers stack allocation.
 */
template <co_allocable T>
struct [[nodiscard("This object should be co_awaited")]] co_new_t<T, std::dynamic_extent> {
  std::size_t count; ///< The number of elements to allocate.
};

/**
 * @brief An awaitable (in the context of an ``lf::task``) which triggers stack deallocation.
 */
template <co_allocable T, std::size_t Extent>
struct [[nodiscard("This object should be co_awaited")]] co_delete_t : std::span<T, Extent> {};

} // namespace impl

inline namespace core {

/**
 * @brief A function which returns an awaitable (in the context of an ``lf::task``) which triggers allocation
 * on a stack's stack.
 *
 * Upon ``co_await``ing the result of this function a pointer or ``std::span`` representing the allocated
 * memory is returned. The memory is deleted with a matched call to ``co_delete``, This must be performed in a
 * FILO order (i.e destroyed in the reverse order they were allocated) with any other ``co_new``ed calls. The
 * lifetime of `co_new`ed memory must strictly nest within the lifetime of the task/co-routine it is allocated
 * within. Finally all calls to ``co_new``/``co_delete`` must occur __outside__ of a fork-join scope.
 *
 * \rst
 *
 * .. warning::
 *    This is an expert only feature with many foot-guns attached.
 *
 * \endrst
 *
 */
template <co_allocable T, std::size_t Extent = std::dynamic_extent>
  requires (Extent == std::dynamic_extent)
inline auto co_new(std::size_t count) -> impl::co_new_t<T, std::dynamic_extent> {
  return impl::co_new_t<T, std::dynamic_extent>{count};
}

/**
 * @brief A function which returns an awaitable (in the context of an ``lf::task``) which triggers allocation
 * on a stack's stack.
 *
 * See the documentation for the dynamic version, ``lf::co_new(std::size_t)``, for a full description.
 */
template <co_allocable T, std::size_t Extent = 1>
  requires (Extent != std::dynamic_extent)
inline auto co_new() -> impl::co_new_t<T, Extent> {
  return {};
}

/**
 * @brief Free the memory allocated by a call to ``co_new``.
 */
template <co_allocable T, std::size_t Extent = 1>
  requires (Extent >= 2)
inline auto co_delete(std::span<T, Extent> span) -> impl::co_delete_t<T, Extent> {
  return impl::co_delete_t<T, Extent>{span};
}

/**
 * @brief Free the memory allocated by a call to ``co_new``.
 */
template <co_allocable T, std::size_t Extent = 1>
  requires (Extent == 1)
inline auto co_delete(T *ptr) -> impl::co_delete_t<T, Extent> {
  return impl::co_delete_t<T, Extent>{std::span<T, 1>{ptr, 1}};
}

} // namespace core

} // namespace lf

#endif /* A951FB73_0FCF_4B7C_A997_42B7E87D21CB */

#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility>
#ifndef A5349E86_5BAA_48EF_94E9_F0EBF630DE04
#define A5349E86_5BAA_48EF_94E9_F0EBF630DE04

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
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
#include <optional>
#include <type_traits>
#include <utility>
#ifndef AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172
#define AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>


/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

inline namespace core {

// --------------------------------- Task --------------------------------- //

// TODO: private destructor such that tasks can only be created inside the library?

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` a reference or a `std::movable` type.
 */
template <typename T>
concept returnable = std::is_void_v<T> || std::is_reference_v<T> || std::movable<T>;

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other coroutines and specify `T` the
 * async function's return type which is required to be `void`, a reference, or a `std::movable` type.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should never touch an instance of this type, it is used for specifying the
 *    return type of an `async` function only.
 *
 * .. warning::
 *    The value type ``T`` of a coroutine should be independent of the coroutines first-argument.
 *
 * \endrst
 */
template <returnable T = void>
struct LF_CORO_ATTRIBUTES task : std::type_identity<T> {
  void *prom; ///< An opaque handle to the coroutine promise.
};

} // namespace core

} // namespace lf

#endif /* AB8DC4EC_1EB3_4FFB_9A05_4D8A99CFF172 */



/**
 * @file eventually.hpp
 *
 * @brief Classes for delaying construction of an object.
 */

namespace lf {

inline namespace core {

// ------------------------------------------------------------------------ //

/**
 * @brief A wrapper to delay construction of an object, like ``std::optional``.
 *
 * This class supports delayed construction of reference types. It is like a simplified version of
 * `std::optional` that is only constructed once. Construction is done (zero or one times) via assignment.
 *
 * \rst
 *
 * .. note::
 *    This documentation is generated from the non-reference specialization, see the source
 *    for the reference specialization.
 *
 * \endrst
 */
template <impl::non_void T>
class eventually : impl::immovable<eventually<T>> {

  std::optional<T> m_value; ///< The contained object.

 public:
  /**
   * @brief Start lifetime of object at assignment.
   */
  template <typename U>
    requires std::constructible_from<T, U>
  void operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) {
    LF_ASSERT(!m_value);
    m_value.emplace(std::forward<U>(expr));
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() noexcept -> T * {
    LF_ASSERT(m_value);
    return m_value.operator->();
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator->() const noexcept -> T const * {
    LF_ASSERT(m_value);
    return m_value.operator->();
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() & noexcept -> T & {
    LF_ASSERT(m_value);
    return *m_value;
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const & noexcept -> T const & {
    LF_ASSERT(m_value);
    return *m_value;
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() && noexcept -> T && {
    LF_ASSERT(m_value);
    return *std::move(m_value);
  }

  /**
   * @brief Access the contained object, must have been constructed first.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T const && {
    LF_ASSERT(m_value);
    return *std::move(m_value);
  }
};

// ------------------------------------------------------------------------ //

/**
 * @brief Has pointer semantics.
 *
 * `eventually<T &> val` should behave like `T & val` except assignment rebinds.
 */
template <impl::non_void T>
  requires impl::reference<T>
class eventually<T> : impl::immovable<eventually<T>> {
 public:
  /**
   * @brief Construct an object inside the eventually from ``expr``.
   */
  template <impl::safe_ref_bind_to<T> U>
  void operator=(U &&expr) noexcept {
    m_value = std::addressof(expr);
  }

  /**
   * @brief Access the wrapped reference.
   */
  [[nodiscard]] auto operator->() const noexcept -> std::remove_reference_t<T> * { return m_value; }

  /**
   * @brief Deference the wrapped pointer.
   *
   * This will decay `T&&` to `T&` just like using a `T &&` reference would.
   */
  [[nodiscard]] auto operator*() const & noexcept -> std::remove_reference_t<T> & { return *m_value; }

  /**
   * @brief Forward the wrapped reference.
   *
   * This will not decay T&& to T&, nor will it promote T& to T&&.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T {
    if constexpr (std::is_rvalue_reference_v<T>) {
      return std::move(*m_value);
    } else {
      return *m_value;
    }
  }

 private:
  std::remove_reference_t<T> *m_value;
};

} // namespace core

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */
#ifndef DD0B4328_55BD_452B_A4A5_5A4670A6217B
#define DD0B4328_55BD_452B_A4A5_5A4670A6217B

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>




/**
 * @file first_arg.hpp
 *
 * @brief Machinery for the (library-generated) first argument of async functions.
 */

namespace lf {

inline namespace core {

/**
 * @brief Test if the expression `*std::declval<T&>()` is valid and has a referenceable type i.e. non-void.
 */
template <typename I>
concept dereferenceable = requires (I val) {
  { *val } -> impl::can_reference;
};

/**
 * @brief A quasi-pointer if a movable type that can be dereferenced to a referenceable type type i.e.
 * non-void.
 *
 * A quasi-pointer is assumed to be cheap-to-move like an iterator/legacy-pointer.
 */
template <typename I>
concept quasi_pointer = std::default_initializable<I> && std::movable<I> && dereferenceable<I>;

/**
 * @brief A concept that requires a type be a copyable [function
 * object](https://en.cppreference.com/w/cpp/named_req/FunctionObject).
 *
 * An async function object is a function object that returns an `lf::task` when `operator()` is called.
 * with appropriate arguments. The call to `operator()` must create a libfork coroutine. The first argument
 * of an async function must accept a deduced templated-type that satisfies the `lf::core::first_arg` concept.
 * The return type and invocability of an async function must be independent of the first argument except
 * for its tag value.
 *
 * An async function may be copied, its copies must be equivalent to the original and support concurrent
 * invocation from multiple threads. It is assumed that an async function is cheap-to-copy like
 * an iterator/legacy-pointer.
 */
template <typename F>
concept async_function_object = std::is_object_v<F> && std::copy_constructible<F>;

/**
 * @brief This describes the public-API of the first argument passed to an async function.
 *
 * An async functions' invocability and return type must be independent of their first argument except for its
 * tag value. A user may query the first argument's static member `tagged` to obtain this value. Additionally,
 * a user may query the first argument's static member function `context()` to obtain a pointer to the current
 * workers `lf::context`. Finally a user may cache an exception in-flight by calling `.stash_exception()`.
 */
template <typename T>
concept first_arg = async_function_object<T> && requires (T arg) {
  { T::tagged } -> std::convertible_to<tag>;
  { T::context() } -> std::same_as<context *>;
  { arg.stash_exception() } noexcept;
};

} // namespace core

namespace impl {

/**
 * @brief The type passed as the first argument to async functions.
 *
 * Its functions are:
 *
 * - Act as a y-combinator (expose same invocability as F).
 * - Provide a handle to the coroutine frame for exception handling.
 * - Statically inform the return pointer type.
 * - Statically provide the tag.
 * - Statically provide the calling argument types.
 */
template <quasi_pointer I, tag Tag, async_function_object F, typename... Cargs>
class first_arg_t {
 public:
  static constexpr tag tagged = Tag; ///< The way this async function was called.

  first_arg_t() = default;

  static auto context() -> context * { return tls::context(); }

  /**
   * @brief Stash an exception that will be rethrown at the end of the next join.
   */
  void stash_exception() const noexcept {
#if LF_COMPILER_EXCEPTIONS
    m_frame->capture_exception();
#endif
  }

  template <different_from<first_arg_t> T>
    requires std::constructible_from<F, T>
  explicit first_arg_t(T &&expr) noexcept(std::is_nothrow_constructible_v<F, T>)
      : m_fun(std::forward<T>(expr)) {}

  template <typename... Args>
    requires std::invocable<F &, Args...>
  auto operator()(Args &&...args) & noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F &, Args...> {
    return std::invoke(m_fun, std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::invocable<F const &, Args...>
  auto operator()(Args &&...args) const & noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F const &, Args...> {
    return std::invoke(m_fun, std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::invocable<F &&, Args...>
  auto operator()(Args &&...args) && noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F &&, Args...> {
    return std::invoke(std::move(m_fun), std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::invocable<F const &&, Args...>
  auto operator()(Args &&...args) const && noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F const &&, Args...> {
    return std::invoke(std::move(m_fun), std::forward<Args>(args)...);
  }

 private:
  /**
   * @brief Hidden friend reduces discoverability, this is an implementation detail.
   */
  friend auto unwrap(first_arg_t &&arg) noexcept -> F && { return std::move(arg.m_fun); }

  /**
   * @brief Hidden friend reduces discoverability, this is an implementation detail.
   */
  LF_FORCEINLINE friend auto unsafe_set_frame(first_arg_t &arg, frame *frame) noexcept {
#if LF_COMPILER_EXCEPTIONS
    arg.m_frame = frame;
#endif
  }

  [[no_unique_address]] F m_fun;
#if LF_COMPILER_EXCEPTIONS
  frame *m_frame;
#endif
};

} // namespace impl

} // namespace lf

#endif /* DD0B4328_55BD_452B_A4A5_5A4670A6217B */



/**
 * @file invocable.hpp
 *
 * @brief A collection concepts that extend ``std::invocable`` to async functions.
 */

namespace lf {

namespace impl {

/**
 * @brief A type which can be assigned any value as a noop.
 *
 * Useful to ignore a value tagged with ``[[no_discard]]``.
 */
struct ignore_t {
  constexpr void operator=([[maybe_unused]] auto const &discard) const noexcept {}
};

/**
 * @brief A tag type to indicate an async function's return value will be discarded by the caller.
 *
 * This type is indirectly writable from any value.
 */
struct discard_t {
  constexpr auto operator*() -> ignore_t { return {}; }
};

// ------------ Bare-bones inconsistent invocable ------------ //

namespace detail {

template <typename I, typename Task>
struct valid_return : std::false_type {};

template <>
struct valid_return<discard_t, task<void>> : std::true_type {};

template <returnable R, std::indirectly_writable<R> I>
struct valid_return<I, task<R>> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that `Task` is an lf::task and that the result can be returned by `I`.
 *
 * This requires that `I` is `std::indirectly_writable` or that `I` is `discard_t` and the task returns void.
 */
template <typename I, typename Task>
inline constexpr bool valid_return_v = quasi_pointer<I> && detail::valid_return<I, Task>::value;

/**
 * @brief Verify that `R` returned via `I`.
 *
 * This requires that `I` is `std::indirectly_writable` or that `I` is `discard_t` and the `R` is void.
 */
template <typename I, typename R>
concept return_address_for = quasi_pointer<I> && returnable<R> && valid_return_v<I, task<R>>;

/**
 * @brief Verify `F` is async `Tag` invocable with `Args...` and returns a task who's result type is
 * returnable via I.
 */
template <typename I, tag Tag, typename F, typename... Args>
concept async_invocable_to_task =
    quasi_pointer<I> &&                                                                                    //
    async_function_object<F> &&                                                                            //
    std::invocable<F, impl::first_arg_t<I, Tag, F, Args &&...>, Args...> &&                                //
    valid_return_v<I, std::invoke_result_t<F, impl::first_arg_t<discard_t, Tag, F, Args &&...>, Args...>>; //

/**
 * @brief Let `F(Args...) -> task<R>` then this returns 'R'.
 *
 * Unsafe in the sense that it does not check that F is `async_invocable`.
 */
template <typename I, tag Tag, typename F, typename... Args>
  requires async_invocable_to_task<I, Tag, F, Args...>
using unsafe_result_t =
    typename std::invoke_result_t<F, impl::first_arg_t<I, Tag, F, Args &&...>, Args...>::type;

// --------------------- //

/**
 * @brief Check that F can be 'Tag'-invoked to produce `task<R>`.
 */
template <typename R, typename I, tag T, typename F, typename... Args>
concept return_exactly =                                //
    async_invocable_to_task<I, T, F, Args...> &&        //
    std::same_as<R, unsafe_result_t<I, T, F, Args...>>; //

/**
 * @brief Check that `F` can be `T`-invoked and called with `Args...`.
 */
template <typename R, typename I, tag T, typename F, typename... Args>
concept call_consistent =                        //
    return_exactly<R, I, T, F, Args...> &&       //
    return_exactly<R, I, tag::call, F, Args...>; //

namespace detail {

template <typename R>
struct as_eventually : std::type_identity<eventually<R> *> {};
template <>
struct as_eventually<void> : std::type_identity<discard_t> {};

} // namespace detail

/**
 * @brief Check that `Tag`-invoking and calling `F` with `Args...` produces task<R>.
 *
 * This also checks the results is consistent when it is discarded and returned by `eventually<R> *`.
 */
template <typename R, typename I, tag T, typename F, typename... Args>
concept self_consistent =                                                       //
    call_consistent<R, I, T, F, Args...> &&                                     //
    call_consistent<R, discard_t, T, F, Args...> &&                             //
    call_consistent<R, typename detail::as_eventually<R>::type, T, F, Args...>; //

// --------------------- //

// /**
//  * @brief Let `F(Args...) -> task<R>` then this returns `eventually<R> *` or `discard_t` if `R` is void.
//  */
// template <typename I, tag Tag, typename F, typename... Args>
//   requires async_invocable_to_task<I, Tag, F, Args...>
// using as_eventually_t = detail::as_eventually<impl::unsafe_result_t<I, Tag, F, Args...>>::type;

/**
 * @brief Check `F` is async invocable to a task with `I`,` discard_t` and the appropriate `eventually`.
 */
template <typename I, tag Tag, typename F, typename... Args>
concept consistent_invocable =                                                //
    async_invocable_to_task<I, Tag, F, Args...> &&                            //
    self_consistent<unsafe_result_t<I, Tag, F, Args...>, I, Tag, F, Args...>; //

// --------------------- //

} // namespace impl

inline namespace core {

/**
 * @brief Check `F` is `Tag`-invocable with `Args...` and returns an `lf::task` who's result is returnable via
 * `I`.
 *
 * In the following description "invoking" or "async invoking" means to call `F` with `Args...` via the
 * appropriate libfork function i.e. `fork` corresponds to `lf::fork[r, f](args...)` and the library will
 * generate the appropriate (opaque) first-argument.
 *
 * This requires:
 *  - `F` is 'Tag'/call invocable with `Args...` when writing the result to `I` or discarding it.
 *  - The result of all of these calls has the same type.
 *  - The result of all of these calls is an instance of type `lf::task<R>`.
 *  - `I` is movable and dereferenceable.
 *  - `I` is indirectly writable from `R` or `R` is `void` while `I` is `discard_t`.
 *  - If `R` is non-void then `F` is `lf::core::async_invocable` when `I` is `lf::[manual_]eventually<R> *`.
 *
 * This concept is provided as a building block for higher-level concepts.
 */
template <typename I, tag Tag, typename F, typename... Args>
concept async_invocable = impl::consistent_invocable<I, Tag, F, Args...>;

// --------- //

/**
 * @brief Alias for `lf::core::async_invocable<lf::impl::discard_t, lf::core::tag::call, F, Args...>`.
 */
template <typename F, typename... Args>
concept invocable = async_invocable<impl::discard_t, tag::call, F, Args...>;

/**
 * @brief Alias for `lf::core::async_invocable<lf::impl::discard_t, lf::core::tag::root, F, Args...>`,
 * subsumes `lf::core::invocable`.
 */
template <typename F, typename... Args>
concept rootable = invocable<F, Args...> && async_invocable<impl::discard_t, tag::root, F, Args...>;

/**
 * @brief Alias for `lf::core::async_invocable<lf::impl::discard_t, lf::core::tag::fork, F, Args...>`,
 * subsumes `lf::core::invocable`.
 */
template <typename F, typename... Args>
concept forkable = invocable<F, Args...> && async_invocable<impl::discard_t, tag::fork, F, Args...>;

// --------- //

/**
 * @brief Fetch `R` when the async function `F` returns `lf::task<R>`.
 */
template <typename F, typename... Args>
  requires invocable<F, Args...>
using async_result_t = impl::unsafe_result_t<impl::discard_t, tag::call, F, Args...>;

} // namespace core

} // namespace lf

#endif /* A5349E86_5BAA_48EF_94E9_F0EBF630DE04 */

#ifndef AD9A2908_3043_4CEC_9A2A_A57DE168DF19
#define AD9A2908_3043_4CEC_9A2A_A57DE168DF19

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits>
#include <utility>

#ifndef CF3E6AC4_246A_4131_BF7A_FE5CD641A19B
#define CF3E6AC4_246A_4131_BF7A_FE5CD641A19B

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>
#include <span>
#include <type_traits>




/**
 * @file awaitables.hpp
 *
 * @brief Awaitables (in a `libfork` coroutine) that trigger a switch, fork, call or join.
 */

namespace lf::impl {

// -------------------------------------------------------- //

/**
 * @brief An awaiter to explicitly transfer execution to another worker.
 *
 * This is generated by `await_transform` when awaiting on a pointer to a `lf::context`.
 */
struct switch_awaitable : std::suspend_always {

  /**
   * @brief Shortcut if already on context.
   */
  auto await_ready() const noexcept { return tls::context() == dest; }

  /**
   * @brief Reschedule this coro onto `dest`.
   */
  auto await_suspend(std::coroutine_handle<>) noexcept -> std::coroutine_handle<> {

    // Schedule this coro for execution on Dest.
    dest->submit(&self);

    // Dest will take this stack upon resumption hence, we must release it.
    ignore_t{} = tls::stack()->release();

    // Eventually dest will fail to pop() the ancestor task that we 'could' pop() here and
    // then treat it as a task that was stolen from it.

    // Now we have a number of tasks on our WSQ which we have "effectively stolen" from dest.
    // All of them will eventually reach a join point.

    // We can pop() the ancestor, mark it stolen and then resume it.

    /**
     * While running the ancestor several things can happen:
     *  - We hit a join in the ancestor:
     *    - Case Win join:
     *      Take stack, OK to treat tasks on our WSQ as non-stolen.
     *    - Case Loose join:
     *      Must treat tasks on our WSQ as stolen.
     * - We loose a join in a descendent of the ancestor:
     *   Ok all task on WSQ must have been stole by other threads and handled as stolen appropriately.
     */

    if (auto *eff_stolen = std::bit_cast<frame *>(tls::context()->pop())) {
      eff_stolen->fetch_add_steal();
      return eff_stolen->self();
    }
    return std::noop_coroutine();
  }

  intrusive_list<submit_handle>::node self; ///< The current coroutine's handle.
  context *dest;                            ///< Target context.
};

// -------------------------------------------------------- //

/**
 * @brief An awaiter that returns space allocated on the current fibre's stack.
 *
 * This never suspends the coroutine and is generated by `await_transform` when awaiting on a pointer to a
 * `lf::context`.
 */
template <typename T, std::size_t E>
struct alloc_awaitable : std::suspend_never, std::span<T, E> {
  /**
   * @brief Return a handle to the memory.
   */
  [[nodiscard]] auto await_resume() const noexcept -> std::conditional_t<E == 1, T *, std::span<T, E>> {
    if constexpr (E == 1) {
      return this->data();
    } else {
      return *this;
    }
  }
};

// -------------------------------------------------------- //

/**
 * @brief An awaiter that suspends the current coroutine and transfers control to a child task.
 *
 * The parent task is made available for stealing. This is generated by `await_transform` when awaiting on an
 * `lf::impl::quasi_awaitable`.
 */
struct fork_awaitable : std::suspend_always {
  /**
   * @brief Sym-transfer to child, push parent to queue.
   */
  auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Forking, push parent to context");
    // Need a copy (on stack) in case *this is destructed after push.
    std::coroutine_handle child = this->child->self();
    tls::context()->push(std::bit_cast<task_handle>(parent));
    return child;
  }

  frame *child;  ///< The suspended child coroutine's frame.
  frame *parent; ///< The calling coroutine's frame.
};

/**
 * @brief An awaiter that suspends the current coroutine and transfers control to a child task.
 *
 * The parent task is __not__ made available for stealing. This is generated by `await_transform` when
 * awaiting on an `lf::impl::quasi_awaitable`.
 */
struct call_awaitable : std::suspend_always {
  /**
   * @brief Sym-transfer to child.
   */
  auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Calling");
    return child->self();
  }

  frame *child; ///< The suspended child coroutine's frame.
};

// -------------------------------------------------------------------------------- //

/**
 * @brief An awaiter to synchronize execution of child tasks.
 *
 * This is generated by `await_transform` when awaiting on an `lf::impl::join_type`.
 */
struct join_awaitable {
 private:
  void take_stack_reset_frame() const noexcept {
    // Steals have happened so we cannot currently own this tasks stack.
    LF_ASSERT(self->load_steals() != 0);
    LF_ASSERT(tls::stack()->empty());
    *tls::stack() = stack{self->stacklet()};
    // Some steals have happened, need to reset the control block.
    self->reset();
  }

 public:
  /**
   * @brief Shortcut if children are ready.
   */
  auto await_ready() const noexcept -> bool {
    // If no steals then we are the only owner of the parent and we are ready to join.
    if (self->load_steals() == 0) {
      LF_LOG("Sync ready (no steals)");
      // Therefore no need to reset the control block.
      return true;
    }
    // Currently:            joins() = k_u16_max - num_joined
    // Hence:       k_u16_max - joins() = num_joined

    // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
    // better if we see all the decrements to joins() and avoid suspending
    // the coroutine if possible. Cannot fetch_sub() here and write to frame
    // as coroutine must be suspended first.
    auto joined = k_u16_max - self->load_joins(std::memory_order_acquire);

    if (self->load_steals() == joined) {
      LF_LOG("Sync is ready");
      take_stack_reset_frame();
      return true;
    }

    LF_LOG("Sync not ready");
    return false;
  }

  /**
   * @brief Mark at join point then yield to scheduler or resume if children are done.
   */
  auto await_suspend(std::coroutine_handle<> task) const noexcept -> std::coroutine_handle<> {
    // Currently        joins  = k_u16_max  - num_joined
    // We set           joins  = joins()    - (k_u16_max - num_steals)
    //                         = num_steals - num_joined

    // Hence               joined = k_u16_max - num_joined
    //         k_u16_max - joined = num_joined

    auto steals = self->load_steals();
    auto joined = self->fetch_sub_joins(k_u16_max - steals, std::memory_order_release);

    if (steals == k_u16_max - joined) {
      // We set joins after all children had completed therefore we can resume the task.
      // Need to acquire to ensure we see all writes by other threads to the result.
      std::atomic_thread_fence(std::memory_order_acquire);
      LF_LOG("Wins join race");
      take_stack_reset_frame();
      return task;
    }
    LF_LOG("Looses join race");

    // Someone else is responsible for running this task.
    // We cannot touch *this or deference self as someone may have resumed already!
    // We cannot currently own this stack (checking would violate above).

    // If no explicit scheduling then we must have an empty WSQ as we stole this task.

    if (auto *eff_stolen = std::bit_cast<frame *>(tls::context()->pop())) {
      eff_stolen->fetch_add_steal();
      return eff_stolen->self();
    }
    return std::noop_coroutine();
  }

  /**
   * @brief Propagate exceptions.
   */
  void await_resume() const {
    LF_LOG("join resumes");
    // Check we have been reset.
    LF_ASSERT(self->load_steals() == 0);
    LF_ASSERT_NO_ASSUME(self->load_joins(std::memory_order_acquire) == k_u16_max);
    LF_ASSERT(self->stacklet() == tls::stack()->top());

    self->rethrow_if_exception();
  }

  frame *self; ///< The frame of the awaiting coroutine.
};

} // namespace lf::impl

#endif /* CF3E6AC4_246A_4131_BF7A_FE5CD641A19B */


/**
 * @file combinate.hpp
 *
 * @brief Utility for building a coroutine's first argument and invoking it.
 */

namespace lf::impl {

// ---------------------------- //

template <returnable R, return_address_for<R> I, tag Tag>
struct promise;

// -------------------------------------------------------- //

/**
 * @brief Awaitable in the context of an `lf::task` coroutine.
 *
 * This will be transformed by an `await_transform` and trigger a fork or call.
 */
template <returnable R, return_address_for<R> I, tag Tag>
struct [[nodiscard("A quasi_awaitable MUST be immediately co_awaited!")]] quasi_awaitable {
  promise<R, I, Tag> *prom; ///< The parent/semaphore needs to be set!
};

// ---------------------------- //

/**
 * @brief Call an async function with a synthesized first argument.
 *
 * The first argument will contain a copy of the function hence, this is a fixed-point combinator.
 */
template <quasi_pointer I, tag Tag, async_function_object F>
struct [[nodiscard("A bound function SHOULD be immediately invoked!")]] y_combinate {

  [[no_unique_address]] I ret; ///< The return address.
  [[no_unique_address]] F fun; ///< The asynchronous function.

  /**
   * @brief Invoke the coroutine, set's the return pointer.
   */
  template <typename... Args>
    requires async_invocable<I, Tag, F, Args...>
  auto operator()(Args &&...args) && -> quasi_awaitable<async_result_t<F, Args...>, I, Tag> {

    task task = std::move(fun)(                                 //
        first_arg_t<I, Tag, F, Args &&...>(std::as_const(fun)), //
        std::forward<Args>(args)...                             //
    );

    using R = async_result_t<F, Args...>;
    using P = promise<R, I, Tag>;

    auto *prom = static_cast<P *>(task.prom);

    if constexpr (!std::is_void_v<R>) {
      prom->set_return(std::move(ret));
    }

    return {prom};
  }
};

// ---------------------------- //

/**
 * @brief Build a combinator for `ret` and `fun`.
 */
template <tag Tag, quasi_pointer I, async_function_object F>
  requires std::is_rvalue_reference_v<I &&>
auto combinate(I &&ret, F fun) -> y_combinate<I, Tag, F> {
  return {std::move(ret), std::move(fun)};
}

/**
 * @brief Build a combinator for `ret` and `fun`.
 *
 * This specialization prevents each layer wrapping the function in another `first_arg_t`.
 */
template <tag Tag,
          tag OtherTag,
          quasi_pointer I,
          quasi_pointer OtherI,
          async_function_object F,
          typename... Args>
  requires std::is_rvalue_reference_v<I &&>
auto combinate(I &&ret, first_arg_t<OtherI, OtherTag, F, Args...> arg) -> y_combinate<I, Tag, F> {
  return {std::move(ret), unwrap(std::move(arg))};
}

} // namespace lf::impl

#endif /* AD9A2908_3043_4CEC_9A2A_A57DE168DF19 */


/**
 * @file control_flow.hpp
 *
 * @brief Meta header which includes ``lf::fork``, ``lf::call``, ``lf::join`` machinery.
 */

namespace lf {

namespace impl {
/**
 * @brief A empty tag type used to disambiguate a join.
 */
struct join_type {};

} // namespace impl

inline namespace core {

/**
 * @brief An awaitable (in a `lf::task`) that triggers a join.
 *
 * After a join is resumed it is guaranteed that all forked child tasks will have completed.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no relationship between the thread that executes the ``lf::join``
 *    and the thread that resumes the coroutine.
 *
 * \endrst
 */
inline constexpr impl::join_type join = {};

} // namespace core

namespace impl {

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  #define LF_DEPRECATE [[deprecated("Use operator[] instead of operator()")]]
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
  template <quasi_pointer I, async_function_object F>
  LF_DEPRECATE [[nodiscard]] LF_STATIC_CALL auto operator()(I ret, F fun) LF_STATIC_CONST {
    return combinate<Tag>(std::move(ret), std::move(fun));
  }

  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <async_function_object F>
  LF_DEPRECATE [[nodiscard]] LF_STATIC_CALL auto operator()(F fun) LF_STATIC_CONST {
    return combinate<Tag>(discard_t{}, std::move(fun));
  }

#if defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <quasi_pointer I, async_function_object F>
  [[nodiscard]] LF_STATIC_CALL auto operator[](I ret, F fun) LF_STATIC_CONST {
    return combinate<Tag>(std::move(ret), std::move(fun));
  }

  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <async_function_object F>
  [[nodiscard]] LF_STATIC_CALL auto operator[](F fun) LF_STATIC_CONST {
    return combinate<Tag>(discard_t{}, std::move(fun));
  }
#endif
};

#undef LF_DEPRECATE

} // namespace impl

inline namespace core {

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 *
 * Conceptually the forked/child task can be executed anywhere at anytime and
 * and in parallel with its continuation.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no guaranteed relationship between the thread that executes the ``lf::fork``
 *    and the thread(s) that execute the continuation/child. However, currently ``libfork``
 *    uses continuation stealing so the thread that calls ``lf::fork`` will immediately begin
 *    executing the child.
 *
 * \endrst
 */
inline constexpr impl::bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 *
 * Conceptually the called/child task can be executed anywhere at anytime but, its
 * continuation is guaranteed to be sequenced after the child returns.
 *
 * \rst
 *
 * .. note::
 *
 *    There is no relationship between the thread that executes the ``lf::call`` and
 *    the thread(s) that execute the continuation/child. However, currently ``libfork``
 *    uses continuation stealing so the thread that calls ``lf::call`` will immediately
 *    begin executing the child.
 *
 * \endrst
 */
inline constexpr impl::bind_task<tag::call> call = {};

} // namespace core

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */
#ifndef AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A
#define AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>

#include <exception>
#include <type_traits>
#include <utility>




/**
 * @file sync_wait.hpp
 *
 * @brief Functionally to enter coroutines from a non-worker thread.
 */

namespace lf {

inline namespace core {

/**
 * @brief A concept that schedulers must satisfy.
 *
 * This requires only a single method, `schedule` which accepts an `lf::intruded_list<submit_handle>` and
 * promises to call `lf::resume()` on it.
 */
template <typename Sch>
concept scheduler = requires (Sch &&sch, intruded_list<submit_handle> handle) {
  std::forward<Sch>(sch).schedule(handle); //
};

/**
 * @brief Schedule execution of `fun` on `sch` and __block__ until the task is complete.
 *
 * This is the primary entry point from the synchronous to the asynchronous world. A typical libfork program
 * is expected to make a call from `main` into a scheduler/runtime by scheduling a single root-task with this
 * function.
 *
 * This will build a task from `fun` and dispatch it to `sch` via its `schedule` method. Sync wait should
 * __not__ be called by a worker thread (which are never allowed to block) unless the call to `schedule`
 * completes synchronously.
 */
template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
auto sync_wait(Sch &&sch, F fun, Args &&...args) -> async_result_t<F, Args...> {

  using R = async_result_t<F, Args...>;
  constexpr bool is_void = std::is_void_v<R>;

  impl::root_notify notifier;
  eventually<std::conditional_t<is_void, impl::empty, R>> result;

  // This is to support a worker sync waiting on work they will launch inline.
  bool worker = impl::tls::has_stack;
  // Will cache workers stack here.
  std::optional<impl::stack> prev = std::nullopt;

  impl::y_combinate combinator = [&]() {
    if constexpr (is_void) {
      return combinate<tag::root>(impl::discard_t{}, std::move(fun));
    } else {
      return combinate<tag::root>(&result, std::move(fun));
    }
  }();

  if (!worker) {
    LF_LOG("Sync wait from non-worker thread");
    impl::tls::thread_stack.construct();
    impl::tls::has_stack = true;
  } else {
    LF_LOG("Sync wait from worker thread");
    prev.emplace();                        // Default construct.
    swap(*prev, *impl::tls::thread_stack); // ADL call.
  }

  // This makes a coroutine which can only be destroyed at final_suspend
  // hence, no exceptions until it has run.
  impl::quasi_awaitable await = std::move(combinator)(std::forward<Args>(args)...);

  [&]() noexcept {
    //
    await.prom->set_root_notify(&notifier);
    auto *handle = std::bit_cast<submit_handle>(static_cast<impl::frame *>(await.prom));

    impl::ignore_t{} = impl::tls::thread_stack->release();

    if (!worker) {
      impl::tls::thread_stack.destroy();
      impl::tls::has_stack = false;
    } else {
      swap(*prev, *impl::tls::thread_stack);
    }

    typename intrusive_list<submit_handle>::node node{handle};

    std::forward<Sch>(sch).schedule(&node);
    notifier.sem.acquire();
  }();

  if (notifier.m_eptr) {
    std::rethrow_exception(std::move(notifier.m_eptr));
  }

  if constexpr (!is_void) {
    return *std::move(result);
  }
}

} // namespace core

} // namespace lf

#endif /* AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A */

#ifndef DE9399DB_593B_4C5C_A9D7_89B9F2FAB920
#define DE9399DB_593B_4C5C_A9D7_89B9F2FAB920

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.



/**
 * @file resume.hpp
 *
 * @brief Functions to resume stolen and submitted task.
 */

namespace lf {

inline namespace ext {

/**
 * @brief Resume a task at a submission point.
 */
inline void resume(submit_handle ptr) noexcept {

  LF_LOG("Call to resume on submitted task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  *impl::tls::stack() = impl::stack{frame->stacklet()};

  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
  frame->self().resume();
  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
}

/**
 * @brief Resume a stolen task.
 */
inline void resume(task_handle ptr) noexcept {

  LF_LOG("Call to resume on stolen task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  frame->fetch_add_steal();

  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
  frame->self().resume();
  LF_ASSERT_NO_ASSUME(impl::tls::context()->empty());
}

} // namespace ext

} // namespace lf

#endif /* DE9399DB_593B_4C5C_A9D7_89B9F2FAB920 */

#ifndef C854CDE9_1125_46E1_9E2A_0B0006BFC135
#define C854CDE9_1125_46E1_9E2A_0B0006BFC135

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <exception>
#include <new>
#include <type_traits>
#include <utility>


#ifndef A896798B_7E3B_4854_9997_89EA5AE765EB
#define A896798B_7E3B_4854_9997_89EA5AE765EB

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>



/**
 * @file return.hpp
 *
 * @brief A promise base class that provides the `return_value` method(s).
 */

namespace lf::impl {

/**
 * @brief A helper class that stores a quasi_pointer.
 */
template <quasi_pointer I>
class return_result_base {
 public:
  void set_return(I &&ret) noexcept { this->m_ret = std::move(ret); }

 protected:
  [[no_unique_address]] I m_ret; ///< The stored quasi-pointer
};

/**
 * @brief General case for non-void, non-reference
 */
template <returnable R, return_address_for<R> I>
struct return_result : return_result_base<I> {

  /**
   * @brief Convert and assign `value` to the return address.
   *
   * If the return address is directly assignable from `value` this will not construct the intermediate `T`.
   */
  template <std::convertible_to<R> U>
  void return_value(U &&value) {
    if constexpr (std::indirectly_writable<I, U>) {
      *(this->m_ret) = std::forward<U>(value);
    } else {
      *(this->m_ret) = static_cast<R>(std::forward<U>(value));
    }
  }

  /**
   * @brief For use with `co_return {expr}`
   */
  void return_value(R &&value) { *(this->m_ret) = std::move(value); }
};

/**
 * @brief Case for reference types.
 */
template <returnable R, return_address_for<R> I>
  requires std::is_reference_v<R>
struct return_result<R, I> : return_result_base<I> {
  /**
   * @brief Assign `value` to the return address.
   */
  template <safe_ref_bind_to<R> U>
  void return_value(U &&ref) {
    *(this->m_ret) = std::forward<U>(ref);
  }
};

/**
 * @brief Case for void return.
 */
template <>
struct return_result<void, discard_t> {
  static constexpr void return_void() noexcept {};
};

} // namespace lf::impl

#endif /* A896798B_7E3B_4854_9997_89EA5AE765EB */


/**
 * @file promise.hpp
 *
 * @brief The promise type for all tasks/coroutines.
 */

namespace lf::impl {

namespace detail {

inline auto final_await_suspend(frame *parent) noexcept -> std::coroutine_handle<> {

  full_context *context = tls::context();

  if (task_handle parent_task = context->pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
    LF_LOG("Parent not stolen, keeps ripping");
    LF_ASSERT(byte_cast(parent_task) == byte_cast(parent));
    // This must be the same thread that created the parent so it already owns the stack.
    // No steals have occurred so we do not need to call reset().;
    return parent->self();
  }

  /**
   * An owner is a worker who:
   *
   * - Created the task.
   * - Had the task submitted to them.
   * - Won the task at a join.
   *
   * An owner of a task owns the stack the task is on.
   *
   * As the worker who completed the child task this thread owns the stack the child task was on.
   *
   * Either:
   *
   * 1. The parent is on the same stack as the child.
   * 2. The parent is on a different stack to the child.
   *
   * Case (1) implies: we owned the parent; forked the child task; then the parent was then stolen.
   * Case (2) implies: we stole the parent task; then forked the child; then the parent was stolen.
   *
   * In case (2) the workers stack has no allocations on it.
   */

  LF_LOG("Task's parent was stolen");

  stack *tls_stack = tls::stack();

  stack::stacklet *p_stacklet = parent->stacklet(); //
  stack::stacklet *c_stacklet = tls_stack->top();   // Need to call while we own tls_stack.

  // Register with parent we have completed this child task, this may release ownership of our stack.
  if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
    // Acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    // Parent has reached join and we are the last child task to complete.
    // We are the exclusive owner of the parent therefore, we must continue parent.

    LF_LOG("Task is last child to join, resumes parent");

    if (p_stacklet != c_stacklet) {
      // Case (2), the tls_stack has no allocations on it.

      LF_ASSERT(tls_stack->empty());

      // TODO: stack.splice()? Here the old stack is empty and thrown away, if it is larger
      // then we could splice it onto the parents one? Or we could attempt to cache the old one.
      *tls_stack = stack{p_stacklet};
    }

    // Must reset parents control block before resuming parent.
    parent->reset();

    return parent->self();
  }

  // We did not win the join-race, we cannot deference the parent pointer now as
  // the frame may now be freed by the winner.

  // Parent has not reached join or we are not the last child to complete.
  // We are now out of jobs, must yield to executor.

  LF_LOG("Task is not last to join");

  if (p_stacklet == c_stacklet) {
    // We are unable to resume the parent and where its owner, as the resuming
    // thread will take ownership of the parent's we must give it up.
    LF_LOG("Thread releases control of parent's stack");

    ignore_t{} = tls_stack->release();

  } else {
    // Case (2) the tls_stack has no allocations on it, it may be used later.
  }

  return std::noop_coroutine();
}

} // namespace detail

/**
 * @brief Type independent bits
 */
struct promise_base : frame {

  using frame::frame;

  /**
   * @brief Allocate the coroutine on a new stack.
   */
  LF_FORCEINLINE static auto operator new(std::size_t size) -> void * { return tls::stack()->allocate(size); }

  /**
   * @brief Deallocate the coroutine from current `stack`s stack.
   */
  LF_FORCEINLINE static void operator delete(void *ptr) noexcept { tls::stack()->deallocate(ptr); }

  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  // -------------------------------------------------------------- //

  template <co_allocable T, std::size_t E>
  auto await_transform(co_new_t<T, E> await) {

    auto *stack = tls::stack();

    T *ptr = static_cast<T *>(stack->allocate(await.count * sizeof(T)));

    // clang-format off

    LF_TRY {
      std::ranges::uninitialized_default_construct_n(ptr, await.count);
    } LF_CATCH_ALL {
      stack->deallocate(ptr);
      LF_RETHROW;
    }

    // clang-format on

    this->reset_stacklet(stack->top());

    return alloc_awaitable<T, E>{{}, std::span<T, E>{ptr, await.count}};
  }

  template <co_allocable T, std::size_t E>
  auto await_transform(co_delete_t<T, E> await) noexcept -> std::suspend_never {
    std::ranges::destroy(await);
    auto *stack = impl::tls::stack();
    stack->deallocate(await.data());
    this->reset_stacklet(stack->top());
    return {};
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Transform a context pointer into a context-switch awaitable.
   */
  auto await_transform(context *dest) -> switch_awaitable {

    auto *submit = std::bit_cast<submit_handle>(static_cast<frame *>(this));

    return {{}, typename intrusive_list<submit_handle>::node{submit}, dest};
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Get a join awaitable.
   */
  auto await_transform(join_type) noexcept -> join_awaitable { return {this}; }

  // -------------------------------------------------------------- //

  /**
   * @brief Transform a call packet into a call awaitable.
   */
  template <returnable R2, return_address_for<R2> I2, tag Tg>
    requires (Tg == tag::call || Tg == tag::fork)
  auto await_transform(quasi_awaitable<R2, I2, Tg> awaitable) noexcept {

    awaitable.prom->set_parent(this);

    if constexpr (Tg == tag::call) {
      return call_awaitable{{}, awaitable.prom};
    }

    if constexpr (Tg == tag::fork) {
      return fork_awaitable{{}, awaitable.prom, this};
    }
  }
};

/**
 * @brief The promise type for all tasks/coroutines.
 *
 * @tparam R The type of the return address.
 * @tparam T The value type of the coroutine (what it promises to return).
 * @tparam Context The type of the context this coroutine is running on.
 * @tparam Tag The dispatch tag of the coroutine.
 */
template <returnable R, return_address_for<R> I, tag Tag>
struct promise : promise_base, return_result<R, I> {

  /**
   * @brief Construct a new promise object, delegate to main constructor.
   */
  template <typename This, first_arg Arg, typename... Args>
  promise(This const & /*unused*/, Arg &arg, Args const &.../*unused*/) noexcept : promise(arg) {}

  /**
   * @brief Construct a new promise object.
   *
   * Stores a handle to the promise in the `frame` and loads the tls stack
   * and stores a pointer to the top fibril. Also sets the first argument's frame pointer.
   */
  template <first_arg Arg, typename... Args>
  explicit promise(Arg &arg, Args const &.../*unused*/) noexcept
      : promise_base{std::coroutine_handle<promise>::from_promise(*this), tls::stack()->top()} {
    unsafe_set_frame(arg, this);
  }

  /**
   * @brief Returned task stores a copy of the `this` pointer.
   */
  auto get_return_object() noexcept -> task<R> { return {{}, static_cast<void *>(this)}; }

  /**
   * @brief Try to resume the parent.
   */
  auto final_suspend() const noexcept {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the stack_stack this child is on

    LF_ASSERT(this->load_steals() == 0);                                           // Fork without join.
    LF_ASSERT_NO_ASSUME(this->load_joins(std::memory_order_acquire) == k_u16_max); // Invalid state.
    LF_ASSERT(!this->has_exception());                                             // Must have rethrown.

    return final_awaitable{};
  }

  /**
   * @brief Cache in parent's stacklet.
   */
  void unhandled_exception() noexcept {
    if constexpr (Tag == tag::root) {
      this->notifier()->m_eptr = std::current_exception();
    } else {
      this->parent()->capture_exception();
    }
  }

 private:
  struct final_awaitable : std::suspend_always {
    static auto await_suspend(std::coroutine_handle<promise> child) noexcept -> std::coroutine_handle<> {

      if constexpr (Tag == tag::root) {

        LF_LOG("Root task at final suspend, releases semaphore and yields");

        child.promise().notifier()->sem.release();
        child.destroy();

        // A root task is always the first on a stack, now it has been completed the stack is empty.
        LF_ASSERT(tls::stack()->empty());

        return std::noop_coroutine();
      }

      LF_LOG("Task reaches final suspend, destroying child");

      frame *parent = child.promise().parent();
      child.destroy();

      if constexpr (Tag == tag::call) {
        LF_LOG("Inline task resumes parent");
        // Inline task's parent cannot have been stolen as its continuation was not
        // pushed to a queue hence, no need to reset control block. We do not
        // attempt to take the stack because stack-eats only occur at a sync point.
        return parent->self();
      }

      return detail::final_await_suspend(parent);
    }
  };
};

// -------------------------------------------------- //

/**
 * @brief A dependent value to emulate `static_assert(false)`.
 */
template <typename...>
inline constexpr bool always_false = false;

/**
 * @brief All non-reference destinations are safe for most types.
 */
template <tag Tag, typename, typename To>
struct safe_fork_t : std::true_type {};

/**
 * @brief Rvalue to const-lvalue promotions are unsafe.
 */
template <typename From, typename To>
struct safe_fork_t<tag::fork, From &&, To const &> : std::false_type {
  static_assert(always_false<From &&, To const &>, "Unsafe r-value to const l-value conversion may dangle!");
};

/**
 * @brief All r-value destinations are always unsafe.
 */
template <typename From, typename To>
struct safe_fork_t<tag::fork, From, To &&> : std::false_type {
  static_assert(always_false<From, To &&>, "Forked r-value may dangle!");
};

/**
 * @brief Triggers a static assert if a conversion may dangle.
 */
template <tag Tag, typename From, typename To>
inline constexpr bool safe_fork_v = safe_fork_t<Tag, From, To>::value;

} // namespace lf::impl

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::returnable R,
          lf::impl::return_address_for<R> I,
          lf::tag Tag,
          lf::async_function_object F,
          typename... Crgs,
          typename... Args>
struct std::coroutine_traits<lf::task<R>, lf::impl::first_arg_t<I, Tag, F, Crgs...>, Args...> {
  // This will trigger an inner static assert if an unsafe reference is forked.
  static_assert((lf::impl::safe_fork_v<Tag, Crgs, Args> && ...));

  using promise_type = lf::impl::promise<R, I, Tag>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::returnable R,
          typename This,
          lf::impl::return_address_for<R> I,
          lf::tag Tag,
          lf::async_function_object F,
          typename... Crgs,
          typename... Args>
struct std::coroutine_traits<lf::task<R>, This, lf::impl::first_arg_t<I, Tag, F, Crgs...>, Args...> {
  // This will trigger an inner static assert if an unsafe reference is forked.
  static_assert((lf::impl::safe_fork_v<Tag, Crgs, Args> && ...));
  static_assert((lf::impl::safe_fork_v<Tag, This, This>), "Object parameter will dangle!");

  using promise_type = lf::impl::promise<R, I, Tag>;
};

#endif

#endif /* C854CDE9_1125_46E1_9E2A_0B0006BFC135 */


/**
 * @file core.hpp
 *
 * @brief Meta header which includes all the headers in ``libfork/core``.
 *
 * This header is a single include which provides the minimal set of headers required
 * for using `libfork` s "core" API. If you are writing your own schedulers and not using any
 * of `libfork` s "extension" API then this is all you need.
 */

#endif /* A6BE090F_9077_40E8_9B57_9BAFD9620469 */


/**
 * @file lift.hpp
 *
 * @brief Higher-order functions for lifting functions into async functions.
 */

namespace lf {

/**
 * @brief A higher-order function that lifts a function into an asynchronous function.
 *
 * \rst
 *
 * This is useful for when you want to fork a regular function:
 *
 * .. code::
 *
 *    auto work(int x) -> int;
 *
 * Then later in some async context you can do:
 *
 * .. code::
 *
 *    {
 *      int a, b;
 *
 *      co_await fork[a, lift](work, 42);
 *      co_await fork[b, lift](work, 007);
 *
 *      co_await join;
 *    }
 *
 * .. note::
 *
 *    The lifted function will accept arguments by forwarding reference.
 *
 * \endrst
 */
inline constexpr auto lift = []<class F, class... Args>(auto, F &&func, Args &&...args)
                                 LF_STATIC_CALL -> task<std::invoke_result_t<F, Args...>>
  requires std::invocable<F, Args...>
{
  co_return std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
};

/**
 * @brief Lift an overload-set/template into a constrained lambda.
 *
 * This is useful for passing overloaded/template names to higher order functions like `lf::fork`/`lf::call`.
 */
#define LF_LOFT(name)                                                                                        \
  [](auto &&...args) LF_STATIC_CALL LF_HOF_RETURNS(name(::std::forward<decltype(args)>(args)...))

/**
 * @brief Lift an overload-set/template into a constrained capturing lambda.
 *
 * The variadic arguments are used as the lambda's capture.
 *
 * This is useful for passing overloaded/template names to higher order functions like `lf::fork`/`lf::call`.
 */
#define LF_CLOFT(name, ...)                                                                                  \
  [__VA_ARGS__](auto &&...args) LF_HOF_RETURNS(name(::std::forward<decltype(args)>(args)...))

} // namespace lf

#endif /* B13463FB_3CF9_46F1_AFAC_19CBCB99A23C */


/**
 * @file algorithm.hpp
 *
 * @brief Meta header which includes all the algorithms in ``libfork/algorithm``.
 */

#endif /* B3512749_D678_438A_8E60_B1E880CF6C23 */
#ifndef A616E976_A92A_4CE4_B807_936EF8C5FBC4
#define A616E976_A92A_4CE4_B807_936EF8C5FBC4

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <exception>
#include <latch>

#include <memory>
#include <numeric>
#include <random>
#include <thread>

#ifndef D8877F11_1F66_4AD0_B949_C0DFF390C2DB
#define D8877F11_1F66_4AD0_B949_C0DFF390C2DB

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cerrno>
#include <climits>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <vector>


/**
 * @file numa.hpp
 *
 * @brief An abstraction over `hwloc`.
 */

#ifdef __has_include
  #if defined(LF_USE_HWLOC) && not __has_include(<hwloc.h>)
    #error "LF_USE_HWLOC is defined but <hwloc.h> is not available"
  #endif
#endif

#ifdef LF_USE_HWLOC
  #include <hwloc.h>
#endif

#ifdef LF_USE_HWLOC
static_assert(HWLOC_VERSION_MAJOR == 2, "hwloc too old");
#endif

/**
 * @brief An opaque description of a set of processing units.
 *
 * \rst
 *
 * .. note::
 *
 *    If your system is missing `hwloc` then this type will be incomplete.
 *
 * \endrst
 */
struct hwloc_bitmap_s;

/**
 * @brief An opaque description of a computers topology.
 *
 * \rst
 *
 * .. note::
 *
 *    If your system is missing `hwloc` then this type will be incomplete.
 *
 * \endrst
 */
struct hwloc_topology;

namespace lf {
inline namespace ext {

// ------------- hwloc can go wrong in a lot of ways... ------------- //

/**
 * @brief An exception thrown when `hwloc` fails.
 */
struct hwloc_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// ------------------------------ Topology decl ------------------------------
// //

/**
 * @brief Enum to control distribution strategy of workers among numa nodes.
 */
enum class numa_strategy {
  fan, // Put workers as far away from each other as possible (maximize cache.)
  seq, // Fill up each numa node sequentially (ignoring SMT).
};

/**
 * @brief A shared description of a computers topology.
 *
 * Objects of this type have shared-pointer semantics.
 */
class numa_topology {

  struct bitmap_deleter {
    LF_STATIC_CALL void operator()(hwloc_bitmap_s *ptr) LF_STATIC_CONST noexcept {
#ifdef LF_USE_HWLOC
      hwloc_bitmap_free(ptr);
#else
      LF_ASSERT(!ptr);
#endif
    }
  };

  using unique_cpup = std::unique_ptr<hwloc_bitmap_s, bitmap_deleter>;

  using shared_topo = std::shared_ptr<hwloc_topology>;

 public:
  /**
   * @brief Construct a topology.
   *
   * If `hwloc` is not installed this topology is empty.
   */
  numa_topology();

  /**
   * @brief Test if this topology is empty.
   */
  explicit operator bool() const noexcept { return m_topology != nullptr; }

  /**
   * A handle to a single processing unit in a NUMA computer.
   */
  class numa_handle {
   public:
    /**
     * @brief Bind the calling thread to the set of processing units in this `cpuset`.
     *
     * If `hwloc` is not installed both handles are null and this is a noop.
     */
    void bind() const;

    shared_topo topo = nullptr; ///< A shared handle to topology this handle belongs to.
    unique_cpup cpup = nullptr; ///< A unique handle to processing units that this handle represents.
    std::size_t numa = 0;       ///< The index of the numa node this handle belongs to, on [0, n).
  };

  /**
   * @brief Split a topology into `n` uniformly distributed handles to single
   * processing units.
   *
   * Here the definition of "uniformly" depends on `strategy`. If `strategy == numa_strategy::seq` we try
   * to use the minimum number of numa nodes then divided each node such that each PU has as much cache as
   * possible. If `strategy == numa_strategy::fan` we try and maximize the amount of cache each PI gets.
   *
   * If this topology is empty then this function returns a vector of `n` empty handles.
   */
  auto split(std::size_t n, numa_strategy strategy = numa_strategy::fan) const -> std::vector<numa_handle>;

  /**
   * @brief A single-threads hierarchical view of a set of objects.
   *
   * This is a `numa_handle` augmented with  list of neighbors-lists each
   * neighbors-list has equidistant neighbors. The first neighbors-list always
   * exists and contains only one element, the one "owned" by the thread. Each
   * subsequent neighbors-list has elements that are topologically more distant
   * from the element in the first neighbour-list.
   */
  template <typename T>
  struct numa_node : numa_handle {
    /**
     * @brief A list of neighbors-lists.
     */
    std::vector<std::vector<std::shared_ptr<T>>> neighbors;
  };

  /**
   * @brief Distribute a vector of objects over this topology.
   *
   * This function returns a vector of `numa_node`s. Each `numa_node` contains a
   * hierarchical view of the elements in `data`.
   */
  template <typename T>
  auto distribute(std::vector<std::shared_ptr<T>> const &data, numa_strategy strategy = numa_strategy::fan)
      -> std::vector<numa_node<T>>;

 private:
  shared_topo m_topology = nullptr;
};

// ---------------------------- Topology implementation ---------------------------- //

#ifdef LF_USE_HWLOC

inline numa_topology::numa_topology() {

  struct topology_deleter {
    LF_STATIC_CALL void operator()(hwloc_topology *ptr) LF_STATIC_CONST noexcept {
      if (ptr != nullptr) {
        hwloc_topology_destroy(ptr);
      }
    }
  };

  hwloc_topology *tmp = nullptr;

  if (hwloc_topology_init(&tmp) != 0) {
    LF_THROW(hwloc_error{"failed to initialize a topology"});
  }

  m_topology = {tmp, topology_deleter{}};

  if (hwloc_topology_load(m_topology.get()) != 0) {
    LF_THROW(hwloc_error{"failed to load a topology"});
  }
}

inline void numa_topology::numa_handle::bind() const {

  LF_ASSERT(topo);
  LF_ASSERT(cpup);

  switch (hwloc_set_cpubind(topo.get(), cpup.get(), HWLOC_CPUBIND_THREAD)) {
    case 0:
      return;
    case -1:
      switch (errno) {
        case ENOSYS:
          LF_THROW(hwloc_error{"hwloc's cpu binding is not supported on this system"});
        case EXDEV:
          LF_THROW(hwloc_error{"hwloc cannot enforce the requested binding"});
        default:
          LF_THROW(hwloc_error{"hwloc cpu bind reported an unknown error"});
      };
    default:
      LF_THROW(hwloc_error{"hwloc cpu bind returned un unexpected value"});
  }
}

inline auto count_cores(hwloc_obj_t obj) -> unsigned int {

  LF_ASSERT(obj);

  if (obj->type == HWLOC_OBJ_CORE) {
    return 1;
  }

  unsigned int num_cores = 0;

  for (unsigned int i = 0; i < obj->arity; i++) {
    num_cores += count_cores(obj->children[i]);
  }

  return num_cores;
}

inline auto get_numa_index(hwloc_topology *topo, hwloc_bitmap_s *bitmap) -> hwloc_uint64_t {

  LF_ASSERT(topo);
  LF_ASSERT(bitmap);

  hwloc_obj *obj = hwloc_get_obj_covering_cpuset(topo, bitmap);

  if (obj == nullptr) {
    LF_THROW(hwloc_error{"failed to find an object covering a bitmap"});
  }

  while (obj != nullptr && obj->memory_arity == 0) {
    obj = obj->parent;
  }

  if (obj == nullptr) {
    LF_THROW(hwloc_error{"failed to find a parent with memory"});
  }

  return obj->gp_index;
}

inline auto numa_topology::split(std::size_t n, numa_strategy strategy) const -> std::vector<numa_handle> {

  if (n < 1) {
    LF_THROW(hwloc_error{"hwloc cannot distribute over less than one singlet"});
  }

  // We are going to build up a list of numa packages until we have enough cores.

  std::vector<hwloc_obj_t> roots;

  if (strategy == numa_strategy::seq) {

    hwloc_obj_t numa = nullptr;

    for (unsigned int count = 0; count < n; count += count_cores(numa)) {

      hwloc_obj_t next_numa = hwloc_get_next_obj_by_type(m_topology.get(), HWLOC_OBJ_PACKAGE, numa);

      if (next_numa == nullptr) {
        break;
      }

      roots.push_back(next_numa);
      numa = next_numa;
    }
  } else {
    roots.push_back(hwloc_get_root_obj(m_topology.get()));
  }

  // Now we distribute over the cores in each numa package, NOTE:  hwloc_distrib
  // gives us owning pointers (not in the docs, but it does!).

  std::vector<hwloc_bitmap_s *> sets(n, nullptr);

  auto r_size = static_cast<unsigned int>(roots.size());
  auto s_size = static_cast<unsigned int>(sets.size());

  if (hwloc_distrib(m_topology.get(), roots.data(), r_size, sets.data(), s_size, INT_MAX, 0) != 0) {
    LF_THROW(hwloc_error{"unknown hwloc error when distributing over a topology"});
  }

  // Need ownership before map for exception safety.
  std::vector<unique_cpup> singlets{sets.begin(), sets.end()};

  std::map<hwloc_uint64_t, std::size_t> numa_map;

  return impl::map(std::move(singlets), [&](unique_cpup &&singlet) -> numa_handle {
    //
    if (!singlet) {
      LF_THROW(hwloc_error{"hwloc_distrib returned a nullptr"});
    }

    if (hwloc_bitmap_singlify(singlet.get()) != 0) {
      LF_THROW(hwloc_error{"unknown hwloc error when singlify a bitmap"});
    }

    hwloc_uint64_t numa_index = get_numa_index(m_topology.get(), singlet.get());

    if (!numa_map.contains(numa_index)) {
      numa_map[numa_index] = numa_map.size();
    }

    return {
        m_topology,
        std::move(singlet),
        numa_map[numa_index],
    };
  });
}

namespace detail {

class distance_matrix {

  using numa_handle = numa_topology::numa_handle;

 public:
  /**
   * @brief Compute the topological distance between all pairs of objects in
   * `obj`.
   */
  explicit distance_matrix(std::vector<numa_handle> const &handles)
      : m_size{handles.size()},
        m_matrix(m_size * m_size) {

    // Transform into hwloc's internal representation of nodes in the topology tree.

    std::vector obj = impl::map(handles, [](numa_handle const &handle) -> hwloc_obj_t {
      return hwloc_get_obj_covering_cpuset(handle.topo.get(), handle.cpup.get());
    });

    for (auto *elem : obj) {
      if (elem == nullptr) {
        LF_THROW(hwloc_error{"failed to find an object covering a handle"});
      }
    }

    // Build the matrix.

    for (std::size_t i = 0; i < obj.size(); i++) {
      for (std::size_t j = 0; j < obj.size(); j++) {

        auto *topo_1 = handles[i].topo.get();
        auto *topo_2 = handles[j].topo.get();

        if (topo_1 != topo_2) {
          LF_THROW(hwloc_error{"numa_handles are in different topologies"});
        }

        hwloc_obj_t ancestor = hwloc_get_common_ancestor_obj(topo_1, obj[i], obj[j]);

        if (ancestor == nullptr) {
          LF_THROW(hwloc_error{"failed to find a common ancestor"});
        }

        int dist_1 = obj[i]->depth - ancestor->depth;
        int dist_2 = obj[j]->depth - ancestor->depth;

        LF_ASSERT(dist_1 >= 0);
        LF_ASSERT(dist_2 >= 0);

        m_matrix[i * m_size + j] = std::max(dist_1, dist_2);
      }
    }
  }

  auto operator()(std::size_t i, std::size_t j) const noexcept -> int { return m_matrix[i * m_size + j]; }

  auto size() const noexcept -> std::size_t { return m_size; }

 private:
  std::size_t m_size;
  std::vector<int> m_matrix;
};

} // namespace detail

template <typename T>
inline auto numa_topology::distribute(std::vector<std::shared_ptr<T>> const &data, numa_strategy strategy)
    -> std::vector<numa_node<T>> {

  std::vector handles = split(data.size(), strategy);

  // Compute the topological distance between all pairs of objects.

  detail::distance_matrix dist{handles};

  std::vector<numa_node<T>> nodes = impl::map(std::move(handles), [](numa_handle &&handle) -> numa_node<T> {
    return {std::move(handle), {}};
  });

  // Compute the neighbors-lists.

  for (std::size_t i = 0; i < nodes.size(); i++) {

    std::set<int> uniques;

    for (std::size_t j = 0; j < nodes.size(); j++) {
      if (i != j) {
        uniques.insert(dist(i, j));
      }
    }

    nodes[i].neighbors.resize(1 + uniques.size());

    for (std::size_t j = 0; j < nodes.size(); j++) {
      if (i == j) {
        nodes[i].neighbors[0].push_back(data[j]);
      } else {
        auto idx = std::distance(uniques.begin(), uniques.find(dist(i, j)));
        LF_ASSERT(idx >= 0);
        nodes[i].neighbors[1 + static_cast<std::size_t>(idx)].push_back(data[j]);
      }
    }
  }

  return nodes;
}

#else

inline numa_topology::numa_topology()
    : m_topology{nullptr, [](hwloc_topology *ptr) {
                   LF_ASSERT(!ptr);
                 }} {}

inline void numa_topology::numa_handle::bind() const {
  LF_ASSERT(!topo);
  LF_ASSERT(!cpup);
}

inline auto numa_topology::split(std::size_t n, numa_strategy /* strategy */) const
    -> std::vector<numa_handle> {
  return std::vector<numa_handle>(n);
}

template <typename T>
inline auto numa_topology::distribute(std::vector<std::shared_ptr<T>> const &data, numa_strategy strategy)
    -> std::vector<numa_node<T>> {

  std::vector<numa_handle> handles = split(data.size(), strategy);

  std::vector<numa_node<T>> views;

  for (std::size_t i = 0; i < data.size(); i++) {

    numa_node<T> node{
        std::move(handles[i]), {{data[i]}}, // The first neighbors-list contains
                                            // only the object itself.
    };

    if (data.size() > 1) {
      node.neighbors.push_back({});
    }

    for (auto const &neigh : data) {
      if (neigh != data[i]) {
        node.neighbors[1].push_back(neigh);
      }
    }

    views.push_back(std::move(node));
  }

  return views;
}

#endif

} // namespace ext

} // namespace lf

#endif /* D8877F11_1F66_4AD0_B949_C0DFF390C2DB */
#ifndef CA0BE1EA_88CD_4E63_9D89_37395E859565
#define CA0BE1EA_88CD_4E63_9D89_37395E859565

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <utility>

/**
 * \file random.hpp
 *
 * @brief Pseudo random number generators (PRNG).
 *
 * This implementation has been adapted from ``http://prng.di.unimi.it/xoshiro256starstar.c``
 */

namespace lf {

namespace impl {

/**
 * @brief A tag type to disambiguated seeding from other operations.
 */
struct seed_t {};

template <typename T>
concept has_result_type = requires { typename std::remove_cvref_t<T>::result_type; };

template <typename G>
concept uniform_random_bit_generator_help =                           //
    std::uniform_random_bit_generator<G> &&                           //
    impl::has_result_type<G> &&                                       //
    std::same_as<std::invoke_result_t<G &>, typename G::result_type>; //

} // namespace impl

inline namespace ext {

inline constexpr impl::seed_t seed = {}; ///< A tag to disambiguate seeding from other operations.

/**
 * @brief `Like std::uniform_random_bit_generator`, but also requires a nested `result_type`.
 */
template <typename G>
concept uniform_random_bit_generator = impl::uniform_random_bit_generator_help<std::remove_cvref_t<G>>;

/**
 * @brief A \<random\> compatible implementation of the xoshiro256** 1.0 PRNG
 *
 * \rst
 *
 * From `the original <https://prng.di.unimi.it/>`_:
 *
 * This is xoshiro256** 1.0, one of our all-purpose, rock-solid generators. It has excellent
 * (sub-ns) speed, a state (256 bits) that is large enough for any parallel application, and it
 * passes all tests we are aware of.
 *
 * \endrst
 */
class xoshiro {
 public:
  using result_type = std::uint64_t; ///< Required by named requirement: UniformRandomBitGenerator

  /**
   * @brief Construct a new xoshiro with a fixed default-seed.
   */
  constexpr xoshiro() = default;

  /**
   * @brief Construct and seed the PRNG.
   *
   * @param my_seed The PRNG's seed, must not be everywhere zero.
   */
  explicit constexpr xoshiro(std::array<result_type, 4> const &my_seed) noexcept : m_state{my_seed} {}

  /**
   * @brief Construct and seed the PRNG from some other generator.
   */
  template <uniform_random_bit_generator PRNG>
    requires (!std::is_const_v<std::remove_reference_t<PRNG &&>>)
  constexpr xoshiro(impl::seed_t, PRNG &&dev) noexcept {
    for (std::uniform_int_distribution<result_type> dist{min(), max()}; auto &elem : m_state) {
      elem = dist(dev);
    }
  }

  /**
   * @brief Get the minimum value of the generator.
   *
   * @return The minimum value that ``xoshiro::operator()`` can return.
   */
  [[nodiscard]] static constexpr auto min() noexcept -> result_type {
    return std::numeric_limits<result_type>::lowest();
  }

  /**
   * @brief Get the maximum value of the generator.
   *
   * @return The maximum value that ``xoshiro::operator()`` can return.
   */
  [[nodiscard]] static constexpr auto max() noexcept -> result_type {
    return std::numeric_limits<result_type>::max();
  }

  /**
   * @brief Generate a random bit sequence and advance the state of the generator.
   *
   * @return A pseudo-random number.
   */
  [[nodiscard]] constexpr auto operator()() noexcept -> result_type {

    result_type const result = rotl(m_state[1] * 5, 7) * 9;
    result_type const temp = m_state[1] << 17; // NOLINT

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= temp;

    m_state[3] = rotl(m_state[3], 45); // NOLINT (magic-numbers)

    return result;
  }

  /**
   * @brief This is the jump function for the generator.
   *
   * It is equivalent to 2^128 calls to operator(); it can be used to generate 2^128 non-overlapping
   * sub-sequences for parallel computations.
   */
  constexpr auto jump() noexcept -> void {
    jump_impl({0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c}); // NOLINT
  }

  /**
   * @brief This is the long-jump function for the generator.
   *
   * It is equivalent to 2^192 calls to operator(); it can be used to generate 2^64 starting points,
   * from each of which jump() will generate 2^64 non-overlapping sub-sequences for parallel
   * distributed computations.
   */
  constexpr auto long_jump() noexcept -> void {
    jump_impl({0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635}); // NOLINT
  }

 private:
  /**
   * @brief The default seed for the PRNG.
   */
  std::array<result_type, 4> m_state = {
      0x8D0B73B52EA17D89,
      0x2AA426A407C2B04F,
      0xF513614E4798928A,
      0xA65E479EC5B49D41,
  };

  /**
   * @brief Utility function.
   */
  [[nodiscard]] static constexpr auto rotl(result_type const val, int const bits) noexcept -> result_type {
    return (val << bits) | (val >> (64 - bits)); // NOLINT
  }

  constexpr void jump_impl(std::array<result_type, 4> const &jump_array) noexcept {
    //
    std::array<result_type, 4> s = {0, 0, 0, 0}; // NOLINT

    for (result_type const jump : jump_array) {
      for (int bit = 0; bit < 64; ++bit) {  // NOLINT
        if (jump & result_type{1} << bit) { // NOLINT
          s[0] ^= m_state[0];
          s[1] ^= m_state[1];
          s[2] ^= m_state[2];
          s[3] ^= m_state[3];
        }
        std::invoke(*this);
      }
    }
    m_state = s;
  }
};

static_assert(uniform_random_bit_generator<xoshiro>);

} // namespace ext

} // namespace lf

#endif /* CA0BE1EA_88CD_4E63_9D89_37395E859565 */

#ifndef C1B42944_8E33_4F6B_BAD6_5FB687F6C737
#define C1B42944_8E33_4F6B_BAD6_5FB687F6C737

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <memory>
#include <random>
#include <vector>



/**
 * @file contexts.hpp
 *
 * @brief An augmentation of the `worker_context` which tracks the topology of the numa nodes.
 */

// --------------------------------------------------------------------- //
namespace lf::impl {

/**
 * @brief Manages an `lf::worker_context` and exposes numa aware stealing.
 */
template <typename Shared>
struct numa_context {
 private:
  static constexpr std::size_t k_min_steal_attempts = 1024;
  static constexpr std::size_t k_steal_attempts_per_target = 32;

  xoshiro m_rng;                                  ///< Thread-local RNG.
  std::shared_ptr<Shared> m_shared;               ///< Shared variables between all numa_contexts.
  worker_context *m_context = nullptr;            ///< The worker context we are associated with.
  std::discrete_distribution<std::size_t> m_dist; ///< The distribution for stealing.
  std::vector<numa_context *> m_close;            ///< First order neighbors.
  std::vector<numa_context *> m_neigh;            ///< Our neighbors (excluding ourselves).

 public:
  /**
   * @brief Construct a new numa context object.
   */
  numa_context(xoshiro const &rng, std::shared_ptr<Shared> shared)
      : m_rng(rng),
        m_shared{std::move(non_null(shared))} {}

  /**
   * @brief Get access to the shared variables.
   */
  [[nodiscard]] auto shared() const noexcept -> Shared & { return *non_null(m_shared); }

  /**
   * @brief An alias for `numa_topology::numa_node<numa_context<Shared>>`.
   */
  using numa_node = numa_topology::numa_node<numa_context>;

  /**
   * @brief Initialize the context and worker with the given topology and bind it to a hardware core.
   *
   * This is separate from construction as the master thread will need to construct
   * the contexts before they can form a reference to them, this must be called by the
   * worker thread which should eventually call `finalize`.
   *
   * The context will store __raw__ pointers to the other contexts in the topology, this is
   * to ensure no circular references are formed.
   *
   * The lifetime of the `context` and `topo` neighbors must outlive all use of this object (excluding
   * destruction).
   */
  void init_worker_and_bind(nullary_function_t notify, numa_node const &topo) {

    LF_ASSERT(!topo.neighbors.empty());
    LF_ASSERT(!topo.neighbors.front().empty());
    LF_ASSERT(topo.neighbors.front().front().get() == this);

    LF_ASSERT(m_neigh.empty()); // Should only be called once.

    topo.bind();

    m_context = worker_init(std::move(notify));

    std::vector<double> weights;

    // clang-format off

    LF_TRY {

      if (topo.neighbors.size() > 1){
        m_close = impl::map(topo.neighbors[1], [](auto const & neigh) {
          return neigh.get();
        });
      }

      // Skip the first one as it is just us.
      for (std::size_t i = 1; i < topo.neighbors.size(); ++i) {

        double n = topo.neighbors[i].size();
        double w = 1 / (n * i * i);

        for (auto &&context : topo.neighbors[i]) {
          weights.push_back(w);
          m_neigh.push_back(context.get());
        }
      }

      m_dist = std::discrete_distribution<std::size_t>{weights.begin(), weights.end()};

    } LF_CATCH_ALL {
      m_close.clear();
      m_neigh.clear();
      LF_RETHROW;
    }
  }

  void finalize_worker() { finalize(std::exchange(m_context, nullptr)); }

  /**
   * @brief Fetch the `lf::context` a thread has associated with this object.
   */
  auto get_underlying() noexcept -> context * { return m_context; }

  /**
   * @brief Submit a job to the owned worker context.
   */
  void submit(lf::intruded_list<lf::submit_handle> jobs) { non_null(m_context)->submit(jobs); }

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   *
   * If there are no submitted tasks, then returned pointer will be null.
   */
  [[nodiscard]] auto try_pop_all() noexcept -> intruded_list<submit_handle> {
    return non_null(m_context)->try_pop_all();
  }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   */
  [[nodiscard]] auto try_steal() noexcept -> task_handle {

    if (m_neigh.empty()){
      return nullptr;
    }

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

    #define LF_RETURN_OR_CONTINUE(expr) \
      auto * context = expr;\
      LF_ASSERT(context); \
      LF_ASSERT(context->m_context);\
      auto [err, task] = context->m_context->try_steal();\
\
      switch (err) {\
        case lf::err::none:\
          LF_LOG("Stole task from {}", (void *)context);\
          return task;\
\
        case lf::err::lost:\
          /* We don't retry here as we don't want to cause contention */ \
          /* and we have multiple steal attempts anyway */ \
        case lf::err::empty:\
          continue;\
\
        default:\
          LF_ASSERT(false && "Unreachable");\
      }


    std::ranges::shuffle(m_close, m_rng);

    // Check all of the closest numa domain.
    for (auto * neigh : m_close) {
      LF_RETURN_OR_CONTINUE(neigh);
    }

    std::size_t attempts = k_min_steal_attempts + k_steal_attempts_per_target * m_neigh.size();

    // Then work probabilistically.
    for (std::size_t i = 0; i < attempts; ++i) {
       LF_RETURN_OR_CONTINUE(m_neigh[m_dist(m_rng)]);
    }

#undef LF_RETURN_OR_CONTINUE

#endif // LF_DOXYGEN_SHOULD_SKIP_THIS

    return nullptr;
  }
};

} // namespace lf::impl

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */


/**
 * @file busy_pool.hpp
 *
 * @brief A work-stealing thread pool where all the threads spin when idle.
 */

namespace lf {

namespace impl {

/**
 * @brief Variable used to synchronize a collection of threads.
 */
struct busy_vars {
  /**
   * @brief Construct a new busy vars object for synchronizing `n` workers with one master.
   */
  explicit busy_vars(std::size_t n) : latch_start(n + 1), latch_stop(n) { LF_ASSERT(n > 0); }

  alignas(k_cache_line) std::latch latch_start; ///< Synchronize construction.
  alignas(k_cache_line) std::latch latch_stop;  ///< Synchronize destruction.
  alignas(k_cache_line) std::atomic_flag stop;  ///< Signal shutdown.
};

/**
 * @brief Workers event-loop.
 */
inline void busy_work(numa_topology::numa_node<impl::numa_context<busy_vars>> node) noexcept {

  LF_ASSERT(!node.neighbors.empty());
  LF_ASSERT(!node.neighbors.front().empty());

  // ------- Initialize my numa variables

  std::shared_ptr my_context = node.neighbors.front().front();

  my_context->init_worker_and_bind(nullary_function_t{[]() {}}, node); // Notification is a no-op.

  // Wait for everyone to have set up their numa_vars. If this throws an exception then
  // program terminates due to the noexcept marker.
  my_context->shared().latch_start.arrive_and_wait();

  LF_DEFER {
    // Wait for everyone to have stopped before destroying the context (which others could be observing).
    my_context->shared().stop.test_and_set(std::memory_order_release);
    my_context->shared().latch_stop.arrive_and_wait();
    my_context->finalize_worker();
  };

  // -------

  while (!my_context->shared().stop.test(std::memory_order_acquire)) {

    intruded_list<submit_handle> submissions = my_context->try_pop_all();

    for_each_elem(submissions, [](lf::submit_handle submitted) LF_STATIC_CALL noexcept {
      resume(submitted);
    });

    if (task_handle task = my_context->try_steal()) {
      resume(task);
    }
  };
}

} // namespace impl

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 * Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class busy_pool : impl::move_only<busy_pool> {

  std::size_t m_num_threads;
  std::uniform_int_distribution<std::size_t> m_dist{0, m_num_threads - 1};
  xoshiro m_rng{seed, std::random_device{}};
  std::shared_ptr<impl::busy_vars> m_share = std::make_shared<impl::busy_vars>(m_num_threads);
  std::vector<std::shared_ptr<impl::numa_context<impl::busy_vars>>> m_worker = {};
  std::vector<std::thread> m_threads = {};
  std::vector<context *> m_contexts = {};

 public:
  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   * @param strategy The numa strategy for distributing workers.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency(),
                     numa_strategy strategy = numa_strategy::fan)
      : m_num_threads(n) {

    for (std::size_t i = 0; i < n; ++i) {
      m_worker.push_back(std::make_shared<impl::numa_context<impl::busy_vars>>(m_rng, m_share));
      m_rng.long_jump();
    }

    LF_ASSERT_NO_ASSUME(!m_share->stop.test(std::memory_order_acquire));

    std::vector nodes = numa_topology{}.distribute(m_worker, strategy);

    [&]() noexcept {
      // All workers must be created, if we fail to create them all then we must terminate else
      // the workers will hang on the start latch.
      for (auto &&node : nodes) {
        m_threads.emplace_back(impl::busy_work, std::move(node));
      }

      // Wait for everyone to have set up their numa_vars before submitting. This
      // must be noexcept as if we fail the countdown then the workers will hang.
      m_share->latch_start.arrive_and_wait();
    }();

    // All workers have set their contexts, we can read them now.
    for (auto &&worker : m_worker) {
      m_contexts.push_back(worker->get_underlying());
    }
  }

  /**
   * @brief Schedule a task for execution.
   */
  void schedule(lf::intruded_list<lf::submit_handle> jobs) { m_worker[m_dist(m_rng)]->submit(jobs); }

  /**
   * @brief Get a view of the worker's contexts.
   */
  auto contexts() noexcept -> std::span<context *> { return m_contexts; }

  ~busy_pool() noexcept {
    LF_LOG("Requesting a stop");
    // Set conditions for workers to stop
    m_share->stop.test_and_set(std::memory_order_release);

    for (auto &worker : m_threads) {
      worker.join();
    }
  }
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */
#ifndef C1BED09D_40CC_4EA1_B687_38A5BCC31907
#define C1BED09D_40CC_4EA1_B687_38A5BCC31907

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <atomic>
#include <bit>
#include <latch>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <thread>


#pragma once

// Copyright (c) Conor Williams, Meta Platforms, Inc. and its affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// The contents of this file have been adapted from https://github.com/facebook/folly

#include <atomic>
#include <bit>
#include <cstdint>
#include <functional>
#include <thread>
#include <type_traits>


/**
 * @file event_count.hpp
 *
 * @brief A standalone adaptation of ``folly::EventCount`` utilizing C++20's atomic wait facilities.
 *
 * This file has been adapted from:
 * ``https://github.com/facebook/folly/blob/main/folly/experimental/EventCount.h``
 */

namespace lf {

inline namespace ext {

/**
 * @brief A condition variable for lock free algorithms.
 *
 *
 *
 * See http://www.1024cores.net/home/lock-free-algorithms/eventcounts for details.
 *
 * Event counts allow you to convert a non-blocking lock-free / wait-free
 * algorithm into a blocking one, by isolating the blocking logic.  You call
 * prepare_wait() before checking your condition and then either cancel_wait()
 * or wait() depending on whether the condition was true.  When another
 * thread makes the condition true, it must call notify() / notify_all() just
 * like a regular condition variable.
 *
 * If "<" denotes the happens-before relationship, consider 2 threads (T1 and
 * T2) and 3 events:
 *
 * * E1: T1 returns from prepare_wait.
 * * E2: T1 calls wait (obviously E1 < E2, intra-thread).
 * * E3: T2 calls ``notify_all()``.
 *
 * If E1 < E3, then E2's wait will complete (and T1 will either wake up,
 * or not block at all)
 *
 * This means that you can use an event_count in the following manner:
 *
 * \rst
 *
 * Waiter:
 *
 * .. code::
 *
 *    if (!condition()) {  // handle fast path first
 *      for (;;) {
 *
 *        auto key = eventCount.prepare_wait();
 *
 *        if (condition()) {
 *          eventCount.cancel_wait();
 *          break;
 *        } else {
 *          eventCount.wait(key);
 *        }
 *      }
 *    }
 *
 * (This pattern is encapsulated in the ``await()`` method.)
 *
 * Poster:
 *
 * .. code::
 *
 *    make_condition_true();
 *    eventCount.notify_all();
 *
 * .. note::
 *
 *    Just like with regular condition variables, the waiter needs to
 *    be tolerant of spurious wakeups and needs to recheck the condition after
 *    being woken up.  Also, as there is no mutual exclusion implied, "checking"
 *    the condition likely means attempting an operation on an underlying
 *    data structure (push into a lock-free queue, etc) and returning true on
 *    success and false on failure.
 *
 * \endrst
 */
class event_count : impl::immovable<event_count> {
 public:
  /**
   * @brief The return type of ``prepare_wait()``.
   */
  class key {
    friend class event_count;
    explicit key(uint32_t epoch) noexcept : m_epoch(epoch) {}
    std::uint32_t m_epoch;
  };
  /**
   * @brief Wake up one waiter.
   */
  auto notify_one() noexcept -> void;
  /**
   * @brief Wake up all waiters.
   */
  auto notify_all() noexcept -> void;
  /**
   * @brief Prepare to wait.
   *
   * Once this has been called, you must either call ``wait()`` with the key or ``cancel_wait()``.
   */
  [[nodiscard]] auto prepare_wait() noexcept -> key;
  /**
   * @brief Cancel a wait that was prepared with ``prepare_wait()``.
   */
  auto cancel_wait() noexcept -> void;
  /**
   * @brief Wait for a notification, this blocks the current thread.
   */
  auto wait(key in_key) noexcept -> void;

  /**
   * Wait for ``condition()`` to become true.
   *
   * Cleans up appropriately if ``condition()`` throws, and then rethrow.
   */
  template <typename Pred>
    requires std::is_invocable_r_v<bool, Pred const &>
  void await(Pred const &condition) noexcept(std::is_nothrow_invocable_r_v<bool, Pred const &>);

 private:
  auto epoch() noexcept -> std::atomic<std::uint32_t> * {
    return reinterpret_cast<std::atomic<std::uint32_t> *>(&m_val) + k_epoch_offset; // NOLINT
  }

  // This requires 64-bit
  static_assert(sizeof(std::uint32_t) == 4, "bad platform, need 32 bit ints");
  static_assert(sizeof(std::uint64_t) == 8, "bad platform, need 64 bit ints");

  static_assert(sizeof(std::atomic<std::uint32_t>) == 4, "bad platform, need 32 bit atomic ints");
  static_assert(sizeof(std::atomic<std::uint64_t>) == 8, "bad platform, need 64 bit atomic ints");

  static constexpr bool k_is_little_endian = std::endian::native == std::endian::little;
  static constexpr bool k_is_big_endian = std::endian::native == std::endian::big;

  static_assert(k_is_little_endian || k_is_big_endian, "bad platform, mixed endian");

  static constexpr size_t k_epoch_offset = k_is_little_endian ? 1 : 0;

  static constexpr std::uint64_t k_add_waiter = 1;
  static constexpr std::uint64_t k_sub_waiter = static_cast<std::uint64_t>(-1);
  static constexpr std::uint64_t k_epoch_shift = 32;
  static constexpr std::uint64_t k_add_epoch = static_cast<std::uint64_t>(1) << k_epoch_shift;
  static constexpr std::uint64_t k_waiter_mask = k_add_epoch - 1;

  // Stores the epoch in the most significant 32 bits and the waiter count in the least significant 32 bits.
  std::atomic<std::uint64_t> m_val = 0;
};

inline void event_count::notify_one() noexcept {
  if (m_val.fetch_add(k_add_epoch, std::memory_order_acq_rel) & k_waiter_mask) [[unlikely]] { // NOLINT
    epoch()->notify_one();
  }
}

inline void event_count::notify_all() noexcept {
  if (m_val.fetch_add(k_add_epoch, std::memory_order_acq_rel) & k_waiter_mask) [[unlikely]] { // NOLINT
    epoch()->notify_all();
  }
}

[[nodiscard]] inline auto event_count::prepare_wait() noexcept -> event_count::key {
  auto prev = m_val.fetch_add(k_add_waiter, std::memory_order_acq_rel);
  // Cast is safe because we're only using the lower 32 bits.
  return key(static_cast<std::uint32_t>(prev >> k_epoch_shift));
}

inline void event_count::cancel_wait() noexcept {
  // memory_order_relaxed would suffice for correctness, but the faster
  // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
  // (and thus system calls).
  auto prev = m_val.fetch_add(k_sub_waiter, std::memory_order_seq_cst);

  LF_ASSERT((prev & k_waiter_mask) != 0);
}

inline void event_count::wait(key in_key) noexcept {
  // Use C++20 atomic wait guarantees
  epoch()->wait(in_key.m_epoch, std::memory_order_acquire);

  // memory_order_relaxed would suffice for correctness, but the faster
  // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
  // (and thus system calls)
  auto prev = m_val.fetch_add(k_sub_waiter, std::memory_order_seq_cst);

  LF_ASSERT((prev & k_waiter_mask) != 0);
}

template <class Pred>
  requires std::is_invocable_r_v<bool, Pred const &>
void event_count::await(Pred const &condition) noexcept(std::is_nothrow_invocable_r_v<bool, Pred const &>) {
  //
  if (std::invoke(condition)) {
    return;
  }
  // std::invoke(condition) is the only thing that may throw, everything else is
  // noexcept, so we can hoist the try/catch block outside of the loop

  LF_TRY {
    for (;;) {
      auto my_key = prepare_wait();
      if (std::invoke(condition)) {
        cancel_wait();
        break;
      }
      wait(my_key);
    }
  }
  LF_CATCH_ALL {
    cancel_wait();
    LF_RETHROW;
  }
}

} // namespace ext

} // namespace lf


/**
 * @file lazy_pool.hpp
 *
 * @brief A work-stealing thread pool where threads sleep when idle.
 */

namespace lf {

namespace impl {

static constexpr std::memory_order acquire = std::memory_order_acquire; ///< Alias
static constexpr std::memory_order acq_rel = std::memory_order_acq_rel; ///< Alias
static constexpr std::memory_order release = std::memory_order_release; ///< Alias

/**
 * @brief A collection of heap allocated atomic variables used for tracking the state of the scheduler.
 */
struct lazy_vars : busy_vars {

  using busy_vars::busy_vars;

  /**
   * @brief Counters and notifiers for each numa locality.
   */
  struct fat_counters {
    alignas(k_cache_line) std::atomic_uint64_t thief = 0; ///< Number of thieving workers.
    alignas(k_cache_line) event_count notifier;           ///< Notifier for this numa pool.
  };

  alignas(k_cache_line) std::atomic_uint64_t active = 0; ///< Total number of actives.
  alignas(k_cache_line) std::vector<fat_counters> numa;  ///< Counters for each numa locality.

  // Invariant: *** if (A > 0) then (T >= 1 OR S == 0) ***

  /**
   * Called by a thief with work, effect: thief->active, do work, active->sleep.
   */
  template <typename Handle>
    requires std::same_as<Handle, task_handle> || std::same_as<Handle, intruded_list<submit_handle>>
  void thief_work_sleep(Handle handle, std::size_t tid) noexcept {

    // Invariant: *** if (A > 0) then (Ti >= 1 OR Si == 0) for all i***

    // First we transition from thief -> sleep:
    //
    // Ti <- Ti - 1
    // Si <- Si + 1
    //
    // Invariant in numa j != i is uneffected.
    //
    // In numa i we guarantee that Ti >= 1 by waking someone else if we are the last thief as Si != 0.

    if (numa[tid].thief.fetch_sub(1, acq_rel) == 1) {
      numa[tid].notifier.notify_one();
    }

    // Then we transition from sleep -> active
    //
    // A <- A + 1
    // Si <- Si - 1

    // If we are the first active then we need to maintain the invariant across all numa domains.

    if (active.fetch_add(1, acq_rel) == 0) {
      for (auto &&domain : numa) {
        if (domain.thief.load(acquire) == 0) {
          domain.notifier.notify_one();
        }
      }
    }

    if constexpr (std::same_as<Handle, intruded_list<submit_handle>>) {
      for_each_elem(handle, [](submit_handle submitted) LF_STATIC_CALL noexcept {
        resume(submitted);
      });
    } else {
      resume(handle);
    }

    // Finally A <- A - 1 does not invalidate the invariant in any domain.
    active.fetch_sub(1, release);
  }
};

/**
 * @brief The function that workers run while the pool is alive (worker event-loop)
 */
inline auto lazy_work(numa_topology::numa_node<numa_context<lazy_vars>> node) noexcept {

  LF_ASSERT(!node.neighbors.empty());
  LF_ASSERT(!node.neighbors.front().empty());

  // ---- Initialization ---- //

  std::shared_ptr my_context = node.neighbors.front().front();

  LF_ASSERT(my_context && !my_context->shared().numa.empty());

  std::size_t numa_tid = node.numa;

  auto &my_numa_vars = my_context->shared().numa[numa_tid]; // node.numa

  lf::nullary_function_t notify{[&my_numa_vars]() {
    my_numa_vars.notifier.notify_all();
  }};

  my_context->init_worker_and_bind(std::move(notify), node);

  // Wait for everyone to have set up their numa_vars. If this throws an exception then
  // program terminates due to the noexcept marker.
  my_context->shared().latch_start.arrive_and_wait();

  LF_DEFER {
    // Wait for everyone to have stopped before destroying the context (which others could be observing).
    my_context->shared().stop.test_and_set(std::memory_order_release);
    my_context->shared().latch_stop.arrive_and_wait();
    my_context->finalize_worker();
  };

  // ----------------------------------- //

  /**
   * Invariant we want to uphold:
   *
   *  If there is an active task there is always: [at least one thief] OR [no sleeping] in each numa.
   *
   * Let:
   *  Ti = number of thieves in numa i
   *  Si = number of sleeping threads in numa i
   *  A = number of active threads across all numa domains
   *
   * Invariant: *** if (A > 0) then (Ti >= 1 OR Si == 0) for all i***
   */

  /**
   * Lemma 1: Promoting an Si -> Ti guarantees that the invariant is upheld.
   *
   * Proof 1:
   *  Ti -> Ti + 1, hence Ti > 0, hence invariant maintained in numa i.
   *  In numa j != i invariant is uneffected.
   *
   */

wake_up:
  /**
   * Invariant maintained by Lemma 1.
   */
  my_numa_vars.thief.fetch_add(1, release);

  /**
   * First we handle the fast path (work to do) before touching the notifier.
   */
  if (auto *submission = my_context->try_pop_all()) {
    my_context->shared().thief_work_sleep(submission, numa_tid);
    goto wake_up;
  }
  if (auto *stolen = my_context->try_steal()) {
    my_context->shared().thief_work_sleep(stolen, numa_tid);
    goto wake_up;
  }

  /**
   * Now we are going to try and sleep if the conditions are correct.
   *
   * Event count pattern:
   *
   *    key <- prepare_wait()
   *
   *    Check condition for sleep:
   *      - We have no private work.
   *      - We are not the watch dog.
   *      - The scheduler has not stopped.
   *
   *    Commit/cancel wait on key.
   */

  auto key = my_numa_vars.notifier.prepare_wait();

  if (auto *submission = my_context->try_pop_all()) {
    // Check our private **before** `stop`.
    my_numa_vars.notifier.cancel_wait();
    my_context->shared().thief_work_sleep(submission, numa_tid);
    goto wake_up;
  }

  if (my_context->shared().stop.test(acquire)) {
    // A stop has been requested, we will honor it under the assumption
    // that the requester has ensured that everyone is done. We cannot check
    // this i.e it is possible a thread that just signaled the master thread
    // is still `active` but act stalled.
    my_numa_vars.notifier.cancel_wait();
    my_numa_vars.notifier.notify_all();
    my_numa_vars.thief.fetch_sub(1, release);
    return;
  }

  /**
   * Try:
   *
   * Ti <- Ti - 1
   * Si <- Si + 1
   *
   * If new Ti == 0 then we check A, if A > 0 then wake self immediately i.e:
   *
   * Ti <- Ti + 1
   * Si <- Si - 1
   *
   * This maintains invariant in numa.
   */

  if (my_numa_vars.thief.fetch_sub(1, acq_rel) == 1) {

    // If we are the last thief then invariant may be broken if A > 0 as S > 0 (because we are asleep).

    if (my_context->shared().active.load(acquire) > 0) {
      // Restore the invariant if A > 0 by immediately waking self.
      my_numa_vars.notifier.cancel_wait();
      goto wake_up;
    }
  }

  LF_LOG("Goes to sleep");

  // We are safe to sleep.
  my_numa_vars.notifier.wait(key);
  // Note, this could be a spurious wakeup, that doesn't matter because we will just loop around.
  goto wake_up;
}

} // namespace impl

/**
 * @brief A scheduler based on a [An Efficient Work-Stealing Scheduler for Task Dependency
 * Graph](https://doi.org/10.1109/icpads51040.2020.00018)
 *
 * This pool sleeps workers which cannot find any work, as such it should be the default choice for most
 * use cases. Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class lazy_pool {

  std::size_t m_num_threads;
  std::uniform_int_distribution<std::size_t> m_dist{0, m_num_threads - 1};
  xoshiro m_rng{seed, std::random_device{}};
  std::shared_ptr<impl::lazy_vars> m_share = std::make_shared<impl::lazy_vars>(m_num_threads);
  std::vector<std::shared_ptr<impl::numa_context<impl::lazy_vars>>> m_worker = {};
  std::vector<std::thread> m_threads = {};
  std::vector<context *> m_contexts = {};

 public:
  /**
   * @brief Construct a new lazy_pool object and `n` worker threads.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   * @param strategy The numa strategy for distributing workers.
   */
  explicit lazy_pool(std::size_t n = std::thread::hardware_concurrency(),
                     numa_strategy strategy = numa_strategy::fan)
      : m_num_threads(n) {

    LF_ASSERT_NO_ASSUME(m_share && !m_share->stop.test(std::memory_order_acquire));

    for (std::size_t i = 0; i < n; ++i) {
      m_worker.push_back(std::make_shared<impl::numa_context<impl::lazy_vars>>(m_rng, m_share));
      m_rng.long_jump();
    }

    std::vector nodes = numa_topology{}.distribute(m_worker, strategy);

    LF_ASSERT(!nodes.empty());

    std::size_t num_numa = 1 + std::ranges::max_element(nodes, {}, [](auto const &node) {
                                 return node.numa;
                               })->numa;

    LF_LOG("Lazy pool has {} numa nodes", num_numa);

    m_share->numa = std::vector<impl::lazy_vars::fat_counters>(num_numa);

    [&]() noexcept {
      // All workers must be created, if we fail to create them all then we must terminate else
      // the workers will hang on the latch.
      for (auto &&node : nodes) {
        m_threads.emplace_back(impl::lazy_work, std::move(node));
      }

      // Wait for everyone to have set up their numa_vars before submitting. This
      // must be noexcept as if we fail the countdown then the workers will hang.
      m_share->latch_start.arrive_and_wait();
    }();

    // All workers have set their contexts, we can read them now.
    for (auto &&worker : m_worker) {
      m_contexts.push_back(worker->get_underlying());
    }
  }

  /**
   * @brief Schedule a job on a random worker.
   */
  void schedule(lf::intruded_list<lf::submit_handle> job) { m_worker[m_dist(m_rng)]->submit(job); }

  /**
   * @brief Get a view of the worker's contexts.
   */
  auto contexts() noexcept -> std::span<context *> { return m_contexts; }

  ~lazy_pool() noexcept {
    LF_LOG("Requesting a stop");

    // Set conditions for workers to stop.
    m_share->stop.test_and_set(std::memory_order_release);

    for (auto &&var : m_share->numa) {
      var.notifier.notify_all();
    }

    for (auto &worker : m_threads) {
      worker.join();
    }
  }
};

static_assert(scheduler<lazy_pool>);

} // namespace lf

#endif /* C1BED09D_40CC_4EA1_B687_38A5BCC31907 */
#ifndef C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC
#define C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


/**
 * @file unit_pool.hpp
 *
 * @brief A scheduler that runs all tasks inline on the current thread.
 */

namespace lf {

/**
 * @brief A scheduler that runs all tasks inline on the current thread.
 *
 * This is useful for testing/debugging/benchmarking.
 */
class unit_pool : impl::immovable<unit_pool> {
 public:
  /**
   * @brief Run a job inline.
   */
  static void schedule(lf::intruded_list<lf::submit_handle> jobs) {
    for_each_elem(jobs, [](lf::submit_handle hand) {
      resume(hand);
    });
  }

  ~unit_pool() noexcept { lf::finalize(m_context); }

 private:
  lf::worker_context *m_context = lf::worker_init(lf::nullary_function_t{[]() {}});
};

} // namespace lf

#endif /* C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC */




/**
 * @file schedule.hpp
 *
 * @brief Meta header which includes all the schedulers in ``libfork/schedule``.
 *
 * Most users of `libfork` will want to use one of the schedulers provided in this header.
 */

#endif /* A616E976_A92A_4CE4_B807_936EF8C5FBC4 */


/**
 * @file libfork.hpp
 *
 * @brief Meta header which includes all of ``libfork``.
 *
 * Users who need all of libfork can use this header, it also serves as the entry-point
 * for the single header generation tool.
 */

#endif /* EDCA974A_808F_4B62_95D5_4D84E31B8911 */
