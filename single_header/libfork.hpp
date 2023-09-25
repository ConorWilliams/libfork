
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
#ifndef A6BE090F_9077_40E8_9B57_9BAFD9620469
#define A6BE090F_9077_40E8_9B57_9BAFD9620469

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
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
#define LF_VERSION_MINOR 2
/**
 * @brief __[public]__ The patch version of libfork.
 *
 * Changes when bug fixes are made in an API backward compatible manner.
 */
#define LF_VERSION_PATCH 0

#ifndef LF_ASYNC_STACK_SIZE
  /**
   * @brief __[public]__ A customizable stack size for ``async_stack``'s (in multiples of 4 kibibytes i.e. the page size).
   *
   * You can override this by defining ``LF_ASYNC_STACK_SIZE`` to a power of two.
   */
  #define LF_ASYNC_STACK_SIZE 256
#endif

static_assert(LF_ASYNC_STACK_SIZE && !(LF_ASYNC_STACK_SIZE & (LF_ASYNC_STACK_SIZE - 1)), "Must be a power of 2");

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
  #define LF_ASSUME(expr)                                                                                                   \
    do {                                                                                                                    \
    } while (false)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  `` `` otherwise ``assert(expr)``.
 *
 * This is for expressions with side-effects.
 */
#ifndef NDEBUG
  #define LF_ASSERT_NO_ASSUME(expr) assert(expr)
#else
  #define LF_ASSERT_NO_ASSUME(expr)
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
  #ifdef LF_DOXYGEN_SHOULD_SKIP_THIS
    #define LF_NOINLINE
  #elif defined(_MSC_VER)
    #define LF_NOINLINE __declspec(noinline)
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #if defined(__CUDACC__)
      // nvcc doesn't always parse __noinline__,
      // see: https://svn.boost.org/trac/boost/ticket/9392
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
 *
 * \endrst
 */
#if !defined(LF_FORCEINLINE)
  #ifdef LF_DOXYGEN_SHOULD_SKIP_THIS
    #define LF_FORCEINLINE
  #elif defined(_MSC_VER)
    #define LF_FORCEINLINE __forceinline
  #elif defined(__GNUC__) && __GNUC__ > 3
    // Clang also defines __GNUC__ (as 4)
    #define LF_FORCEINLINE __attribute__((__always_inline__))
  #else
    #define LF_FORCEINLINE
  #endif
#endif

/**
 * @brief This works-around https://github.com/llvm/llvm-project/issues/63022
 */
#if defined(__clang__)
  #if defined(__apple_build_version__) || __clang_major__ <= 16
    #define LF_TLS_CLANG_INLINE LF_NOINLINE
  #else
    #define LF_TLS_CLANG_INLINE LF_FORCEINLINE
  #endif
#else
  #define LF_TLS_CLANG_INLINE LF_FORCEINLINE
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

    #define LF_LOG(message, ...)                                                                                            \
      do {                                                                                                                  \
        if (!std::is_constant_evaluated()) {                                                                                \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n';                                    \
        }                                                                                                                   \
      } while (false)
  #else
    #define LF_LOG(head, ...)
  #endif
#endif

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
#include <optional>
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

template <typename T>
struct return_nullopt {
  LF_STATIC_CALL constexpr auto operator()() LF_STATIC_CONST noexcept -> std::optional<T> { return {}; }
};

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

/**
 * @brief Basic implementation of a Golang like defer.
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
 */
template <class F>
  requires std::is_nothrow_invocable_v<F>
class [[nodiscard("An instance of defer will execute immediately unless bound to a name!")]] defer : immovable<defer<F>> {
 public:
  /**
   * @brief Construct a new Defer object.
   *
   * @param f Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(F &&f) noexcept(std::is_nothrow_constructible_v<F, F &&>) : m_f(std::forward<F>(f)) {}

  /**
   * @brief Call the invocable.
   */
  constexpr ~defer() noexcept { std::invoke(std::forward<F>(m_f)); }

 private:
  [[no_unique_address]] F m_f;
};

// ---------------- Meta programming ---------------- //

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

template <typename T, typename F>
auto map(std::vector<T> const &from, F &&func) -> std::vector<std::invoke_result_t<F &, T const &>> {

  std::vector<std::invoke_result_t<F &, T const &>> out;

  out.reserve(from.size());

  for (auto &&item : from) {
    out.emplace_back(std::invoke(func, item));
  }

  return out;
}

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
 * @brief Returns ``ptr`` and asserts it is non-null
 */
template <typename T>
constexpr auto non_null(T *ptr) noexcept -> T * {
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
   * @brief An intruded
   */
  class node : impl::immovable<node> {
   public:
    explicit constexpr node(T const &data) : m_data(data) {}

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
    friend constexpr void for_each(node *root, F &&func) noexcept(std::is_nothrow_invocable_v<F, T &>) {
      while (root) {
        // Have to be very careful here, we can't deference `walk` after
        // we've called `func` as `func` could destroy the node.
        auto next = root->m_next;
        std::invoke(func, root->m_data);
        root = next;
      }
    }

   private:
    friend class intrusive_list;

    [[no_unique_address]] T m_data;
    node *m_next;
  };

  /**
   * @brief Push a new node, this can be called concurrently from any number of threads.
   */
  constexpr void push(node *new_node) noexcept {

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
   * such that `for_each` will operate if FIFO order.
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

template <typename T>
using intrusive_node = typename intrusive_list<T>::node;

} // namespace ext

} // namespace lf

#endif /* BC7496D2_E762_43A4_92A3_F2AD10690569 */
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

// NOLINTBEGIN

#ifdef __has_include
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
#else
  #include <coroutine>
namespace lf {
namespace stdx = std;
}
#endif

// NOLINTEND

#endif /* FE9C96B0_5DDD_4438_A3B0_E77BD54F8673 */


/**
 * @file stack.hpp
 *
 * @brief Provides the core elements of the async cactus-stack an thread-local memory.
 *
 * This is almost entirely an implementation detail, the `frame_block *` interface is unsafe.
 */

namespace lf {

// -------------------- async stack -------------------- //

inline namespace ext {

class async_stack;

} // namespace ext

namespace impl {

static constexpr std::size_t k_stack_size = 4 * k_kibibyte * LF_ASYNC_STACK_SIZE;

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
 *
 * These can be alloacted on the heap just like any other object when a `thread_context`
 * needs them.
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

// ----------------------------------------------- //

/**
 * @brief This namespace contains `inline thread_local constinit` variables and functions to manipulate them.
 */
namespace tls {

template <typename Context>
constinit inline thread_local Context *ctx = nullptr;

constinit inline thread_local std::byte *asp = nullptr;

LF_TLS_CLANG_INLINE inline auto get_asp() noexcept -> std::byte * { return asp; }

LF_TLS_CLANG_INLINE inline void set_asp(std::byte *new_asp) noexcept { asp = new_asp; }

template <typename Context>
LF_TLS_CLANG_INLINE inline auto get_ctx() noexcept -> Context * {
  return ctx<Context>;
}

/**
 * @brief Set `tls::asp` to point at `frame`.
 */
template <typename Context>
LF_TLS_CLANG_INLINE inline void push_asp(std::byte *top) {
  LF_LOG("Thread eats a stack");
  LF_ASSERT(tls::asp);
  std::byte *prev = std::exchange(tls::asp, top);
  LF_ASSERT(prev != top);
  async_stack *stack = bytes_to_stack(prev);
  tls::ctx<Context>->stack_push(stack);
}

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
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */
struct frame_block : private immovable<frame_block>, debug_block {
  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks.
   */
  void resume_stolen() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(tls::get_asp());
    m_steal += 1;
    coro().resume();
  }
  /**
   * @brief Resume an external task.
   *
   * When this function returns this worker will have run out of tasks
   * and their asp will be pointing at a sentinel.
   */
  template <typename Context>
  inline void resume_external() noexcept {

    LF_LOG("Call to resume on external task");

    LF_ASSERT(tls::get_asp());

    if (!is_root()) {
      tls::push_asp<Context>(top());
    } else {
      LF_LOG("External was root");
    }

    coro().resume();

    LF_ASSERT(tls::get_asp());
    LF_ASSERT(!non_null(tls::get_ctx<Context>())->task_pop());
  }

// protected:
/**
 * @brief Construct a frame block.
 *
 * Pass ``top == nullptr`` if this is on the heap. Non-root tasks will need to call ``set_parent(...)``.
 */
#ifndef LF_COROUTINE_OFFSET
  frame_block(stdx::coroutine_handle<> coro, std::byte *top) : m_coro{coro}, m_top(top) { LF_ASSERT(coro); }
#else
  frame_block(stdx::coroutine_handle<>, std::byte *top) : m_top(top) {}
#endif

  /**
   * @brief Set the pointer to the parent frame.
   */
  void set_parent(frame_block *parent) noexcept {
    LF_ASSERT(!m_parent);
    m_parent = non_null(parent);
  }

  /**
   * @brief Get a pointer to the parent frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame_block * {
    LF_ASSERT(!is_root());
    return non_null(m_parent);
  }

  /**
   * @brief Get a pointer to the top of the top of the async-stack this frame was allocated on.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto top() const noexcept -> std::byte * {
    LF_ASSERT(!is_root());
    return non_null(m_top);
  }

  struct local_t {
    bool is_root;
    std::byte *top;
  };

  /**
   * @brief Like `is_root()` and `top()` but valid for root frames.
   *
   * Note that if this is a root frame then the pointer to the top of the async-stack has an undefined value.
   */
  [[nodiscard]] auto locale() const noexcept -> local_t { return {is_root(), m_top}; }

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
  [[nodiscard]] auto load_joins(std::memory_order order) const noexcept -> std::uint32_t { return m_join.load(order); }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  auto fetch_sub_joins(std::uint32_t val, std::memory_order order) noexcept -> std::uint32_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] auto steals() const noexcept -> std::uint32_t { return m_steal; }

  /**
   * @brief Check if this is a root frame.
   */
  [[nodiscard]] auto is_root() const noexcept -> bool { return m_parent == nullptr; }

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
    std::construct_at(&m_join, k_u32_max);
  }

 private:
#ifndef LF_COROUTINE_OFFSET
  stdx::coroutine_handle<> m_coro;
#endif

  std::byte *m_top;                        ///< Needs to be separate in-case allocation elided.
  frame_block *m_parent = nullptr;         ///< Same ^
  std::atomic_uint32_t m_join = k_u32_max; ///< Number of children joined (with offset).
  std::uint32_t m_steal = 0;               ///< Number of steals.
};

static_assert(alignof(frame_block) <= k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

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
  //  static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
  //   auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
  //   LF_ASSERT(std::has_single_bit(align));
  //   LF_ASSERT(align > 0);
  //   return std::max(align, impl::k_new_align);
  // }

 protected:
  explicit promise_alloc_stack(stdx::coroutine_handle<> self) noexcept : frame_block{self, tls::get_asp()} {}

 public:
  /**
   * @brief Allocate the coroutine on the current `async_stack`.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] LF_TLS_CLANG_INLINE static auto operator new(std::size_t size) -> void * {
    LF_ASSERT(tls::asp);
    tls::asp -= (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Allocating {} bytes on stack from {}", size, (void *)tls::asp);
    return tls::asp;
  }

  /**
   * @brief Deallocate the coroutine on the current `async_stack`.
   */
  LF_TLS_CLANG_INLINE static void operator delete(void *ptr, std::size_t size) {
    LF_ASSERT(ptr == tls::asp);
    tls::asp += (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Deallocating {} bytes on stack to {}", size, (void *)tls::asp);
  }
};

} // namespace impl

inline namespace ext {

/**
 * @brief A type safe wrapper around a handle to a stealable coroutine.
 *
 * Instances of this type will be passed to a worker's `thread_context`.
 *
 * \rst
 *
 * .. warning::
 *
 *    A pointer to an ``task_h`` must never be dereferenced, only ever passed to ``resume()``.
 *
 * \endrst
 */
template <typename Context>
struct task_h {
  /**
   * @brief Only a worker who has called `worker_init(Context *)` can resume this task.
   */
  friend void resume(task_h *ptr) noexcept {
    LF_ASSERT(impl::tls::get_ctx<Context>());
    LF_ASSERT(impl::tls::get_asp());
    std::bit_cast<impl::frame_block *>(ptr)->resume_stolen();
  }
};

/**
 * @brief A type safe wrapper around a handle to a coroutine that is at a submission point.
 *
 * Instances of this type (wrapped in an `lf::intrusive_list`s node) will be passed to a worker's `thread_context`.
 *
 * \rst
 *
 * .. warning::
 *
 *    A pointer to an ``submit_h`` must never be dereferenced, only ever passed to ``resume()``.
 *
 * \endrst
 */
template <typename Context>
struct submit_h {
  /**
   * @brief Only a worker who has called `worker_init(Context *)` can resume this task.
   */
  friend void resume(submit_h *ptr) noexcept {
    LF_ASSERT(impl::tls::get_ctx<Context>());
    LF_ASSERT(impl::tls::get_asp());
    std::bit_cast<impl::frame_block *>(ptr)->template resume_external<Context>();
  }
};

// --------------------------------------------------------- //

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
template <typename Context>
LF_NOINLINE void worker_init(Context *context) noexcept {

  LF_LOG("Initializing worker");

  LF_ASSERT(context);

  impl::tls::ctx<Context> = context;
  impl::tls::asp = impl::stack_as_bytes(context->stack_pop());
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
template <typename Context>
LF_NOINLINE void worker_finalize(Context *context) {

  LF_LOG("Finalizing worker");

  LF_ASSERT(context == impl::tls::ctx<Context>);
  LF_ASSERT(impl::tls::asp);

  context->stack_push(impl::bytes_to_stack(impl::tls::asp));
}

// ------------------------------------------------- //

} // namespace ext

} // namespace lf

#endif /* C5C3AA77_D533_4A89_8D33_99BD819C1B4C */


/**
 * @file meta.hpp
 *
 * @brief Provides interfaces and meta programming utilities.
 */
namespace lf {

inline namespace ext {

// ----------------------------------------------------- //

/**
 * @brief An alias for a `submit_h<Context> *` stored in a linked list.
 */
template <typename Context>
using intruded_h = intrusive_node<submit_h<Context> *>;

// ------------------------------------------------------ //

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of
 * tasks. The stack of ``lf::async_stack``s is expected to never be empty, it
 * should always be able to return an empty ``lf::async_stack``.
 *
 * Syntactically a `thread_context` requires:
 *
 * \rst
 *
 * .. include:: ../../include/libfork/core/meta.hpp
 *    :code:
 *    :start-line: 56
 *    :end-line: 64
 *
 * \endrst
 */
template <typename Context>
concept thread_context = requires (Context ctx, async_stack *stack, intruded_h<Context> *ext, task_h<Context> *task) {
  { ctx.max_threads() } -> std::same_as<std::size_t>;           // The maximum number of threads.
  { ctx.submit(ext) };                                          // Submit an external task to the context.
  { ctx.task_pop() } -> std::convertible_to<task_h<Context> *>; // If the stack is empty, return a null pointer.
  { ctx.task_push(task) };                                      // Push a non-null pointer.
  { ctx.stack_pop() } -> std::convertible_to<async_stack *>;    // Return a non-null pointer
  { ctx.stack_push(stack) };                                    // Push a non-null pointer
};

namespace detail {

// clang-format off

template <thread_context Context>
static consteval auto always_single_threaded() -> bool {
  if constexpr (requires { Context::max_threads(); }) {
    if constexpr (impl::constexpr_callable<[] { Context::max_threads(); }>) {
      return Context::max_threads() == 1;
    }
  }
  return false;
}

// clang-format on

} // namespace detail

template <typename Context>
concept single_thread_context = thread_context<Context> && detail::always_single_threaded<Context>();

} // namespace ext

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
};

// ------------------------ Helpers ----------------------- //

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

// -------------------------- Forward declaration -------------------------- //

/**
 * @brief Check if an invocable is suitable for use as an `lf::async` function.
 *
 * This requires `T` to be:
 *
 * - A class type.
 * - Trivially copyable.
 * - Default initializable.
 * - Empty.
 */
template <typename T>
concept stateless =
    std::is_class_v<T> && std::is_trivially_copyable_v<T> && std::is_empty_v<T> && std::default_initializable<T>;

// See "async.hpp" for the definition.

template <stateless Fn>
struct [[nodiscard("async functions must be called")]] async;

// -------------------------- First arg static interface -------------------------- //

} // namespace core

namespace impl {

/**
 * @brief Detect what kind of async function a type can be cast to.
 */
template <stateless T>
consteval auto implicit_cast_to_async(async<T>) -> T {
  return {};
}

} // namespace impl

inline namespace core {

/**
 * @brief The API of the first argument passed to an async function.
 *
 * All async functions must have a templated first arguments, this argument will be generated by the compiler and encodes
 * many useful/queryable properties. A full specification is give below:
 *
 * \rst
 *
 * .. include:: ../../include/libfork/core/meta.hpp
 *    :code:
 *    :start-line: 198
 *    :end-line: 218
 *
 * \endrst
 */
template <typename Arg>
concept first_arg = impl::unqualified<Arg> && requires (Arg arg) {
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

  { impl::implicit_cast_to_async(arg) } -> std::same_as<function_of<Arg>>;
};

} // namespace core

namespace impl {

/**
 * @brief The negation of `first_arg`.
 */
template <typename T>
concept not_first_arg = !first_arg<T>;

/**
 * @brief Check if a type is a `first_arg` with a specific tag.
 */
template <typename Arg, tag Tag>
concept first_arg_tagged = first_arg<Arg> && tag_of<Arg> == Tag;

} // namespace impl

} // namespace lf

#endif /* D66BBECE_E467_4EB6_B74A_AAA2E7256E02 */
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


/**
 * @file eventually.hpp
 *
 * @brief A class for delaying construction of an object.
 */

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
 *    It is undefined behavior if the object inside an `eventually` is not constructed before it
 *    is used or if the lifetime of the ``lf::eventually`` ends before an object is constructed.
 *    If you are placing instances of `eventually` on the heap you need to be very careful about exceptions.
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
   * @brief Access the wrapped object as is.
   *
   * This will not decay T&& to T&, nor will it promote T& to T&&.
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
  constexpr eventually() noexcept : m_init{} {}
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
    requires requires (eventually self) { self.emplace(std::forward<U>(expr)); }
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
  [[nodiscard]] constexpr auto operator*() && noexcept(std::is_nothrow_move_constructible_v<T>) -> T {
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


/**
 * @file result.hpp
 *
 * @brief A base class that provides the ``return_[...]`` methods for coroutine promises.
 */

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
 * This type is in the ``core`` namespace as the ``return_[...]`` methods are part of the public API.
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
        *(this->address()) = [&]() -> T {
          return std::forward<U>(value);
        }();
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

#define LF_FWD_ARGS std::forward<strip_rvalue_ref_t<Args>>(margs)...

    if constexpr (impl::non_void<R>) {
      impl::apply_to(static_cast<std::tuple<Args...> &&>(args), [ret = this->address()](Args... margs) {
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


/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` and `lf::async` types.
 */

namespace lf {

inline namespace core {

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other coroutines.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should ever touch an instance of this type, it is used for specifying the
 *    return type of an `async` function only.
 *
 * .. warning::
 *    The value type ``T`` of a coroutine should never be a function of its context or return address type.
 *
 * \endrst
 */
template <typename T = void>
struct task {

  using value_type = T; ///< The type of the value returned by the coroutine.

  /**
   * @brief __Not__ part of the public API.
   *
   * This should only be called by the compiler.
   */
  constexpr task(impl::frame_block *frame) noexcept : m_frame{non_null(frame)} {}

  /**
   * @brief __Not__ part of the public API.
   */
  [[nodiscard]] constexpr auto frame() const noexcept -> impl::frame_block * { return m_frame; }

 private:
  impl::frame_block *m_frame; ///< The frame block for the coroutine.
};

} // namespace core

namespace impl {

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

} // namespace impl

namespace impl {

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
  [[nodiscard]] static auto context() noexcept -> Context * { return non_null(tls::get_ctx<Context>()); }
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
  auto invoke(frame_block *parent) && -> frame_block *requires (tag_of<Head> != tag::root) {
    task_type tsk = std::apply(function_of<Head>{}, std::move(m_args));
    tsk.frame()->set_parent(parent);
    return tsk.frame();
  }

  /**
   * @brief Call the underlying async function with args.
   */
  auto invoke() && -> frame_block *requires (tag_of<Head> == tag::root) {
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

namespace detail {

template <typename Packet>
struct repack {};

template <typename Head, typename... Args>
using swap_head = basic_first_arg<eventually<value_of<packet<Head, Args...>>>, tag::call, function_of<Head>>;

template <first_arg_tagged<tag::invoke> Head, typename... Args>
  requires valid_packet<swap_head<Head, Args...>, Args...>
struct repack<packet<Head, Args...>> : std::type_identity<packet<swap_head<Head, Args...>, Args...>> {
  static_assert(std::is_void_v<return_of<Head>>, "Only void packets are expected to be repacked");
};

} // namespace detail

template <typename Packet>
using repack_t = typename detail::repack<Packet>::type;

/**
 * @brief Check if a void invoke packet with `value_type` `X` can be converted to a call packet with `return_type`
 * `eventually<X>` without changing the `value_type` of the new packet.
 */
template <typename Packet>
concept repackable = non_void<value_of<Packet>> && requires { typename detail::repack<Packet>::type; } &&
                     std::same_as<value_of<Packet>, value_of<repack_t<Packet>>>;

} // namespace impl

// ----------------------------------------------- //

inline namespace core {

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 *
 * Use this alongside `lf::task` to define an synchronous function.
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
   *
   * .. code::
   *
   *    constexpr async fib = [](auto fib, ...) -> task<int> {
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
    requires impl::repackable<invoke_packet<Args...>>
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
  auto max_threads() -> std::size_t;                ///< Unimplemented.
  auto submit(intruded_h<dummy_context> *) -> void; ///< Unimplemented.
  auto task_pop() -> task_h<dummy_context> *;       ///< Unimplemented.
  auto task_push(task_h<dummy_context> *) -> void;  ///< Unimplemented.
  auto stack_pop() -> async_stack *;                ///< Unimplemented.
  auto stack_push(async_stack *) -> void;           ///< Unimplemented.
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
  [[nodiscard]] static auto context() noexcept -> context_type *;
};

/**
 * @brief Specialization for non-void returning task.
 */
template <typename R, tag Tag, stateless F>
struct basic_first_arg : basic_first_arg<void, Tag, F> {

  using return_type = R; ///< The type of the return address.

  constexpr basic_first_arg(return_type &ret) noexcept : m_ret{std::addressof(ret)} {}

  [[nodiscard]] constexpr auto address() const noexcept -> return_type * { return m_ret; }

 private:
  R *m_ret;
};

static_assert(first_arg<basic_first_arg<void, tag::root, decltype([] {})>>);
static_assert(first_arg<basic_first_arg<int, tag::root, decltype([] {})>>);

} // namespace impl

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */
#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility>


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
  LF_DEPRECATE [[nodiscard("A HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()(R &ret, async<F>) LF_STATIC_CONST noexcept {
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
  LF_DEPRECATE [[nodiscard("A HOF needs to be called")]] LF_STATIC_CALL constexpr auto
  operator()(async<F>) LF_STATIC_CONST noexcept {
    return []<typename... Args>(Args &&...args) LF_STATIC_CALL noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
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
  [[nodiscard("A HOF needs to be called")]] static constexpr auto operator[](R &ret, async<F>) noexcept {
    return [&ret]<typename... Args>(Args &&...args) noexcept -> packet<basic_first_arg<R, Tag, F>, Args...> {
      return {{ret}, std::forward<Args>(args)...};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard("A HOF needs to be called")]] static constexpr auto operator[](async<F>) noexcept {
    return []<typename... Args>(Args &&...args) LF_STATIC_CALL noexcept -> packet<basic_first_arg<void, Tag, F>, Args...> {
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
 * @brief The `promise_type` for tasks.
 */

namespace lf::impl {

namespace detail {

// -------------------------------------------------------------------------- //

template <thread_context Context>
struct switch_awaitable {

  auto await_ready() const noexcept { return tls::get_ctx<Context>() == non_null(dest); }

  void await_suspend(stdx::coroutine_handle<>) noexcept { non_null(dest)->submit(&self); }

  void await_resume() const noexcept {}

  intruded_h<Context> self;
  Context *dest;
};

// -------------------------------------------------------------------------- //

template <thread_context Context>
struct fork_awaitable : stdx::suspend_always {
  auto await_suspend(stdx::coroutine_handle<>) const noexcept -> stdx::coroutine_handle<> {
    LF_LOG("Forking, push parent to context");
    m_parent->debug_inc();
    // Need it here (on real stack) in case *this is destructed after push.
    stdx::coroutine_handle child = m_child->coro();
    tls::get_ctx<Context>()->task_push(std::bit_cast<task_h<Context> *>(m_parent));
    return child;
  }
  frame_block *m_parent;
  frame_block *m_child;
};

// -------------------------------------------------------------------------- //

struct call_awaitable : stdx::suspend_always {
  auto await_suspend(stdx::coroutine_handle<>) const noexcept -> stdx::coroutine_handle<> {
    LF_LOG("Calling");
    return m_child->coro();
  }
  frame_block *m_child;
};

// -------------------------------------------------------------------------- //

template <thread_context Context, repackable Packet>
struct invoke_awaitable : stdx::suspend_always {
 private:
  using repack = repack_t<Packet>;

 public:
  auto await_suspend(stdx::coroutine_handle<>) noexcept -> stdx::coroutine_handle<> {

    LF_LOG("Invoking");

    repack new_packet = std::move(m_packet).apply([&](auto, auto &&...args) -> repack {
      return {{m_res}, std::forward<decltype(args)>(args)...};
    });

    return std::move(new_packet).template patch_with<Context>().invoke(parent)->coro();
  }

  [[nodiscard]] auto await_resume() -> value_of<repack> { return *std::move(m_res); }

  frame_block *parent;
  Packet m_packet;
  eventually<value_of<repack>> m_res;
};

// -------------------------------------------------------------------------- //

template <thread_context Context, bool IsRoot>
struct join_awaitable {
 private:
  void take_stack_reset_control() const noexcept {
    // Steals have happened so we cannot currently own this tasks stack.
    LF_ASSERT(self->steals() != 0);

    if constexpr (!IsRoot) {
      tls::push_asp<Context>(self->top());
    }
    // Some steals have happened, need to reset the control block.
    self->reset();
  }

 public:
  auto await_ready() const noexcept -> bool {
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

  auto await_suspend(stdx::coroutine_handle<> task) const noexcept -> stdx::coroutine_handle<> {
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
    LF_LOG("Looses join race");
    // Someone else is responsible for running this task and we have run out of work.
    // We cannot touch *this or deference self as someone may have resumed already!
    // We cannot currently own this stack (checking would violate above).
    return stdx::noop_coroutine();
  }

  void await_resume() const noexcept {
    LF_LOG("join resumes");
    // Check we have been reset.
    LF_ASSERT(self->steals() == 0);
    LF_ASSERT_NO_ASSUME(self->load_joins(std::memory_order_acquire) == k_u32_max);

    self->debug_reset();

    if constexpr (!IsRoot) {
      LF_ASSERT(self->top() == tls::get_asp());
    }
  }

  frame_block *self;
};

// -------------------------------------------------------------------------- //

template <thread_context Context>
auto final_await_suspend(frame_block *parent) noexcept -> std::coroutine_handle<> {

  Context *context = non_null(tls::get_ctx<Context>());

  if (task_h<Context> *parent_task = context->task_pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
    LF_LOG("Parent not stolen, keeps ripping");
    LF_ASSERT(byte_cast(parent_task) == byte_cast(parent));
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

  // Need to copy these onto stack for else branch later.
  auto [is_root, top] = parent->locale();

  // Register with parent we have completed this child task.
  // If we are not the last we cannot deference the parent pointer as frame may now be freed.
  if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
    // Acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    // Parent has reached join and we are the last child task to complete.
    // We are the exclusive owner of the parent therefore, we must continue parent.

    LF_LOG("Task is last child to join, resumes parent");

    if (!is_root) [[likely]] {
      if (top != tls::get_asp()) {
        tls::push_asp<Context>(top);
      }
    }

    // Must reset parents control block before resuming parent.
    parent->reset();

    return parent->coro();
  }

  // Parent has not reached join or we are not the last child to complete.
  // We are now out of jobs, must yield to executor.

  LF_LOG("Task is not last to join");

  if (!is_root) [[likely]] {
    if (top == tls::get_asp()) {
      // We are unable to resume the parent, as the resuming thread will take
      // ownership of the parent's stack we must give it up.
      LF_LOG("Thread releases control of parent's stack");

      tls::set_asp(stack_as_bytes(context->stack_pop()));
    }
  }

  return stdx::noop_coroutine();
}

// -------------------------------------------------------------------------- //

} // namespace detail

/**
 * @brief Use to change a `first_arg` s tag to `tag::call`.
 */
template <first_arg Head>
struct rewrite_tag : Head {
  static constexpr tag tag_value = tag::call; ///< The tag value.
};

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
    static auto await_suspend(stdx::coroutine_handle<promise_type> child) noexcept -> stdx::coroutine_handle<> {

      if constexpr (Tag == tag::root) {
        LF_LOG("Root task at final suspend, releases semaphore and yields");

        // Finishing a root task implies our stack is empty and should have no exceptions.
        child.promise().address()->semaphore.release();
        child.destroy();

        return stdx::noop_coroutine();
      }

      LF_LOG("Task reaches final suspend, destroying child");

      frame_block *parent = child.promise().parent();
      child.destroy();

      if constexpr (Tag == tag::call) {
        LF_LOG("Inline task resumes parent");
        // Inline task's parent cannot have been stolen as its continuation was not pushed to a queue,
        // Hence, no need to reset control block.

        // We do not attempt to push_asp the stack because stack eats only occur at a sync point.
        return parent->coro();
      }
      return detail::final_await_suspend<Context>(parent);
    }
  };

 public:
  /**
   * @brief Construct promise with void return type.
   */
  promise_type() noexcept : allocator<Tag>(stdx::coroutine_handle<promise_type>::from_promise(*this)) {}

  /**
   * @brief Construct promise, sets return address.
   */
  template <first_arg Head, typename... Tail>
  explicit promise_type(Head const &head, Tail const &...)
    requires std::constructible_from<promise_result<R, T>, R *>
      : allocator<Tag>{stdx::coroutine_handle<promise_type>::from_promise(*this)},
        promise_result<R, T>{head.address()} {}

  /**
   * @brief Construct promise, sets return address.
   *
   * For member function coroutines.
   */
  template <not_first_arg Self, first_arg Head, typename... Tail>
  explicit promise_type(Self const &, Head const &head, Tail const &...tail)
    requires std::constructible_from<promise_result<R, T>, R *>
      : promise_type{head, tail...} {}

  auto get_return_object() noexcept -> frame_block * { return this; }

  static auto initial_suspend() noexcept -> stdx::suspend_always { return {}; }

  /**
   * @brief Terminates the program.
   */
  static void unhandled_exception() noexcept {
    noexcept_invoke([] {
      LF_RETHROW;
    });
  }

  /**
   * @brief Try to resume the parent.
   */
  auto final_suspend() const noexcept -> final_awaitable {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the async_stack this child is on

    LF_ASSERT(this->debug_count() == 0);
    LF_ASSERT(this->steals() == 0);                                                // Fork without join.
    LF_ASSERT_NO_ASSUME(this->load_joins(std::memory_order_acquire) == k_u32_max); // Destroyed in invalid state.

    return final_awaitable{};
  }

  /**
   * @brief Transform a context pointer into a context switch awaitable.
   */
  auto await_transform(Context *dest) -> detail::switch_awaitable<Context> {

    auto *fb = static_cast<frame_block *>(this);
    auto *sh = std::bit_cast<submit_h<Context> *>(fb);

    return {intruded_h<Context>{sh}, dest};
  }

  /**
   * @brief Transform a fork packet into a fork awaitable.
   */
  template <first_arg_tagged<tag::fork> Head, typename... Args>
  auto await_transform(packet<Head, Args...> &&packet) noexcept -> detail::fork_awaitable<Context> {
    return {{}, this, std::move(packet).template patch_with<Context>().invoke(this)};
  }

  /**
   * @brief Transform a call packet into a call awaitable.
   */
  template <first_arg_tagged<tag::call> Head, typename... Args>
  auto await_transform(packet<Head, Args...> &&packet) noexcept -> detail::call_awaitable {
    return {{}, std::move(packet).template patch_with<Context>().invoke(this)};
  }

  /**
   * @brief Transform a fork packet into a call awaitable.
   *
   * This subsumes the above `await_transform()` for forked packets if `Context::max_threads() == 1` is true.
   */
  template <first_arg_tagged<tag::fork> Head, typename... Args>
    requires single_thread_context<Context> && valid_packet<rewrite_tag<Head>, Args...>
  auto await_transform(packet<Head, Args...> &&pack) noexcept -> detail::call_awaitable {
    return await_transform(std::move(pack).apply([](Head head, Args &&...args) -> packet<rewrite_tag<Head>, Args...> {
      return {{std::move(head)}, std::forward<Args>(args)...};
    }));
  }

  /**
   * @brief Get a join awaitable.
   */
  auto await_transform(join_type) noexcept -> detail::join_awaitable<Context, Tag == tag::root> { return {this}; }

  /**
   * @brief Transform an invoke packet into an invoke_awaitable.
   */
  template <impl::non_reference Packet>
  auto await_transform(Packet &&pack) noexcept -> detail::invoke_awaitable<Context, Packet> {
    return {{}, this, std::forward<Packet>(pack), {}};
  }
};

// -------------------------------------------------------------------------- //

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


/**
 * @file sync_wait.hpp
 *
 * @brief Utilities for synchronous execution of asynchronous functions.
 */

namespace lf {

inline namespace core {

/**
 * @brief A concept that schedulers must satisfy.
 *
 * This requires only a single method, `schedule` and a nested typedef `context_type`.
 */
template <typename Sch>
concept scheduler = requires (Sch &&sch, intruded_h<context_of<Sch>> *ext) { std::forward<Sch>(sch).schedule(ext); };

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

  impl::frame_block *frame = std::move(packet).invoke();

  intruded_h<context_of<Sch>> link{std::bit_cast<submit_h<context_of<Sch>> *>(frame)};

  LF_LOG("Submitting root");

  // If this throws we crash the program because we cannot know if the eventually in root_block was constructed.
  std::forward<Sch>(sch).schedule(&link);

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
 *
 * This header is a single include which provides the minimal set of headers required
 * for using `libfork` s "core" API. If you are writing your own schedulers and not using any
 * of `libfork` s "extension" API then this is all you need.
 */

#endif /* A6BE090F_9077_40E8_9B57_9BAFD9620469 */
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
#include <memory>
#include <random>
#include <thread>

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

#ifndef C9703881_3D9C_41A5_A7A2_44615C4CFA6A
#define C9703881_3D9C_41A5_A7A2_44615C4CFA6A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>


/**
 * @file deque.hpp
 *
 * @brief A stand-alone, production-quality implementation of the Chase-Lev lock-free
 * single-producer multiple-consumer deque.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A concept that verifies a type is trivially semi-regular and lock-free.
 */
template <typename T>
concept simple =
    std::is_default_constructible_v<T> && std::is_trivially_copyable_v<T> && std::atomic<T>::is_always_lock_free;

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
template <simple T>
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
    (m_buf.get() + (index & m_mask))->store(val, std::memory_order_relaxed); // NOLINT Avoid cast to std::size_t.
  }
  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  [[nodiscard]] constexpr auto load(std::ptrdiff_t index) const noexcept -> T {
    LF_ASSERT(index >= 0);
    return (m_buf.get() + (index & m_mask))->load(std::memory_order_relaxed); // NOLINT Avoid cast to std::size_t.
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
  [[nodiscard]] constexpr auto resize(std::ptrdiff_t bottom, std::ptrdiff_t top) const -> atomic_ring_buf<T> * { // NOLINT
    auto *ptr = new atomic_ring_buf{2 * m_cap};                                                                  // NOLINT
    for (std::ptrdiff_t i = top; i != bottom; ++i) {
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
 * .. include:: ../../test/source/schedule/deque.cpp
 *    :code:
 *    :start-after: // !BEGIN-EXAMPLE
 *    :end-before: // !END-EXAMPLE
 *
 * \endrst
 *
 * @tparam T The type of the elements in the deque.
 * @tparam Optional The type returned by ``pop()``.
 */
template <simple T>
class deque : impl::immovable<deque<T>> {

  static constexpr std::ptrdiff_t k_default_capacity = 1024;
  static constexpr std::size_t k_garbage_reserve = 32;

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
  constexpr void push(T const &val) noexcept;
  /**
   * @brief Pop an item from the deque.
   *
   * Only the owner thread can pop out an item from the deque. If the buffer is empty calls `when_empty` and returns the
   * result. By default, `when_empty` is a no-op that returns a null `std::optional<T>`.
   */
  template <std::invocable F = impl::return_nullopt<T>>
    requires std::convertible_to<T, std::invoke_result_t<F>>
  constexpr auto pop(F &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<F>) -> std::invoke_result_t<F>;

  /**
   * @brief The return type of the ``steal()`` operation.
   *
   * This type is suitable for structured bindings. We return a custom type instead of a
   * ``std::optional`` to allow for more information to be returned as to why a steal may fail.
   */
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
   * @brief Steal an item from the deque.
   *
   * Any threads can try to steal an item from the deque. This operation can fail if the deque is
   * empty or if another thread simultaneously stole an item from the deque.
   */
  constexpr auto steal() noexcept -> steal_t;
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
  alignas(impl::k_cache_line) std::vector<std::unique_ptr<impl::atomic_ring_buf<T>>> m_garbage; // Store old buffers here.

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <simple T>
constexpr deque<T>::deque(std::ptrdiff_t cap) : m_top(0),
                                                m_bottom(0),
                                                m_buf(new impl::atomic_ring_buf<T>{cap}) {
  m_garbage.reserve(k_garbage_reserve);
}

template <simple T>
constexpr auto deque<T>::size() const noexcept -> std::size_t {
  return static_cast<std::size_t>(ssize());
}

template <simple T>
constexpr auto deque<T>::ssize() const noexcept -> std::ptrdiff_t {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return std::max(bottom - top, ptrdiff_t{0});
}

template <simple T>
constexpr auto deque<T>::capacity() const noexcept -> ptrdiff_t {
  return m_buf.load(relaxed)->capacity();
}

template <simple T>
constexpr auto deque<T>::empty() const noexcept -> bool {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return top >= bottom;
}

template <simple T>
constexpr auto deque<T>::push(T const &val) noexcept -> void {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(acquire);
  impl::atomic_ring_buf<T> *buf = m_buf.load(relaxed);

  if (buf->capacity() < (bottom - top) + 1) {
    // deque is full, build a new one.
    m_garbage.emplace_back(std::exchange(buf, buf->resize(bottom, top)));
    m_buf.store(buf, relaxed);
  }

  // Construct new object, this does not have to be atomic as no one can steal this item until
  // after we store the new value of bottom, ordering is maintained by surrounding atomics.
  buf->store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);
}

template <simple T>
template <std::invocable F>
  requires std::convertible_to<T, std::invoke_result_t<F>>
constexpr auto deque<T>::pop(F &&when_empty) noexcept(std::is_nothrow_invocable_v<F>) -> std::invoke_result_t<F> {

  std::ptrdiff_t const bottom = m_bottom.load(relaxed) - 1; //
  impl::atomic_ring_buf<T> *buf = m_buf.load(relaxed);      //
  m_bottom.store(bottom, relaxed);                          // Stealers can no longer steal.

  std::atomic_thread_fence(seq_cst);
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

template <simple T>
constexpr auto deque<T>::steal() noexcept -> steal_t {
  std::ptrdiff_t top = m_top.load(acquire);
  std::atomic_thread_fence(seq_cst);
  std::ptrdiff_t const bottom = m_bottom.load(acquire);

  if (top < bottom) {
    // Must load *before* acquiring the slot as slot may be overwritten immediately after
    // acquiring. This load is NOT required to be atomic even-though it may race with an overwrite
    // as we only return the value if we win the race below guaranteeing we had no race during our
    // read. If we loose the race then 'x' could be corrupt due to read-during-write race but as T
    // is trivially destructible this does not matter.
    T tmp = m_buf.load(consume)->load(top);

    static_assert(std::is_trivially_destructible_v<T>, "concept 'simple' should guarantee this already");

    if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
      return {.code = err::lost, .val = {}};
    }
    return {.code = err::none, .val = tmp};
  }
  return {.code = err::empty, .val = {}};
}

template <simple T>
constexpr deque<T>::~deque() noexcept {
  delete m_buf.load(); // NOLINT
}

} // namespace ext

} // namespace lf

#endif /* C9703881_3D9C_41A5_A7A2_44615C4CFA6A */
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
#include <memory>
#include <set>
#include <vector>


#ifdef LF_HAS_HWLOC
  #include <hwloc.h>
#endif

/**
 * @file numa.hpp
 *
 * @brief An abstraction over `hwloc`.
 */

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

// ------------------------------ Topology decl ------------------------------ //

/**
 * @brief A shared description of a computers topology.
 *
 * Objects of this type have shared-pointer semantics.
 */
class numa_topology {

  struct bitmap_deleter {
    LF_STATIC_CALL void operator()(hwloc_bitmap_s *ptr) LF_STATIC_CONST noexcept {
#ifdef LF_HAS_HWLOC
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
    unique_cpup cpup = nullptr; ///< A unique handle to processing units in `topo` that this handle represents.
  };

  /**
   * @brief Split a topology into `n` uniformly distributed handles to single processing units.
   *
   * Here "uniformly" means they each have as much cache as possible. If this topology is empty then
   * this function returns a vector of `n` empty handles.
   */
  auto split(std::size_t n) const -> std::vector<numa_handle>;

  /**
   * @brief A single-threads hierarchical view of a set of objects.
   *
   * This is a `numa_handle` augmented with  list of neighbors-lists each neighbors-list has
   * equidistant neighbors. The first neighbors-list always exists and contains only one element, the
   * one "owned" by the thread. Each subsequent neighbors-list has elements that are topologically more
   * distant from the element in the first neighbour-list.
   */
  template <typename T>
  struct numa_node : numa_handle {
    std::vector<std::vector<std::shared_ptr<T>>> neighbors; ///< A list of neighbors-lists.
  };

  /**
   * @brief Distribute a vector of objects over this topology.
   *
   * This function returns a vector of `numa_node`s. Each `numa_node` contains a hierarchical view of
   * the elements in `data`.
   */
  template <typename T>
  auto distribute(std::vector<std::shared_ptr<T>> const &data) -> std::vector<numa_node<T>>;

 private:
  shared_topo m_topology = nullptr;
};

// ------------------------------ Topology implementation ------------------------------ //

#ifdef LF_HAS_HWLOC

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

inline auto numa_topology::split(std::size_t n) const -> std::vector<numa_handle> {

  if (n < 1) {
    LF_THROW(hwloc_error{"hwloc cannot distribute over less than one singlet"});
  }

  std::vector<hwloc_bitmap_s *> sets(n, nullptr);

  hwloc_obj_t root = impl::non_null(hwloc_get_root_obj(m_topology.get()));

  // hwloc_distrib gives us owning pointers (not in the docs, but it does!).

  if (hwloc_distrib(m_topology.get(), &root, 1, sets.data(), n, INT_MAX, 0) != 0) {
    LF_THROW(hwloc_error{"unknown hwloc error when distributing over a topology"});
  }

  // Need ownership before map for exception safety.
  std::vector<unique_cpup> singlets{sets.begin(), sets.end()};

  return impl::map(std::move(singlets), [&](unique_cpup &&singlet) -> numa_handle {
    //
    if (!singlet) {
      LF_THROW(hwloc_error{"hwloc_distrib returned a nullptr"});
    }

    if (hwloc_bitmap_singlify(singlet.get()) != 0) {
      LF_THROW(hwloc_error{"unknown hwloc error when singlify a bitmap"});
    }

    return {m_topology, std::move(singlet)};
  });
}

namespace detail {

class distance_matrix {

  using numa_handle = numa_topology::numa_handle;

 public:
  /**
   * @brief Compute the topological distance between all pairs of objects in `obj`.
   */
  explicit distance_matrix(std::vector<numa_handle> const &handles) : m_size{handles.size()}, m_matrix(m_size * m_size) {

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
inline auto numa_topology::distribute(std::vector<std::shared_ptr<T>> const &data) -> std::vector<numa_node<T>> {

  std::vector handles = split(data.size());

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
        nodes[i].neighbors[1 + idx].push_back(data[j]);
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

inline auto numa_topology::split(std::size_t n) const -> std::vector<numa_handle> { return std::vector<numa_handle>(n); }

template <typename T>
inline auto numa_topology::distribute(std::vector<std::shared_ptr<T>> const &data) -> std::vector<numa_node<T>> {

  std::vector<numa_handle> handles = split(data.size());

  std::vector<numa_node<T>> views;

  for (std::size_t i = 0; i < data.size(); i++) {

    numa_node<T> node{
        std::move(handles[i]), {{data[i]}}, // The first neighbors-list contains only the object itself.
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

// See <http://creativecommons.org/publicdomain/zero/1.0/>.

#include <array>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
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

} // namespace impl

inline namespace ext {

inline constexpr impl::seed_t seed = {}; ///< A tag to disambiguate seeding from other operations.

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
   * @param seed The PRNG's seed, must not be everywhere zero.
   */
  explicit constexpr xoshiro(std::array<result_type, 4> const &my_seed) noexcept : m_state{my_seed} {
    if (my_seed == std::array<result_type, 4>{0, 0, 0, 0}) {
      LF_ASSERT(false);
    }
  }

  /**
   * @brief Construct and seed the PRNG from some other generator.
   */
  template <typename PRNG>
    requires requires (PRNG &&device) {
      { std::invoke(device) } -> std::unsigned_integral;
    }
  constexpr xoshiro(impl::seed_t, PRNG &&dev) noexcept : xoshiro({scale(dev), scale(dev), scale(dev), scale(dev)}) {}

  /**
   * @brief Get the minimum value of the generator.
   *
   * @return The minimum value that ``xoshiro::operator()`` can return.
   */
  [[nodiscard]] static constexpr auto min() noexcept -> result_type { return std::numeric_limits<result_type>::lowest(); }

  /**
   * @brief Get the maximum value of the generator.
   *
   * @return The maximum value that ``xoshiro::operator()`` can return.
   */
  [[nodiscard]] static constexpr auto max() noexcept -> result_type { return std::numeric_limits<result_type>::max(); }

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

  /**
   * @brief Utility function to upscale prng's result_type to xoshiro's result_type.
   */
  template <typename PRNG>
    requires requires (PRNG &&device) {
      { std::invoke(device) } -> std::unsigned_integral;
    }
  [[nodiscard]] static constexpr auto scale(PRNG &&device) noexcept -> result_type {
    //
    constexpr std::size_t chars_in_prng = sizeof(std::invoke_result_t<PRNG>);

    constexpr std::size_t chars_in_xoshiro = sizeof(result_type);

    static_assert(chars_in_xoshiro < chars_in_prng || chars_in_xoshiro % chars_in_prng == 0);

    result_type bits = std::invoke(device);

    for (std::size_t i = 1; i < chars_in_xoshiro / chars_in_prng; i++) {
      bits = (bits << CHAR_BIT * chars_in_prng) + std::invoke(device);
    }

    return bits;
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

} // namespace ext

} // namespace lf

#endif /* CA0BE1EA_88CD_4E63_9D89_37395E859565 */
#ifndef C0E5463D_72D1_43C1_9458_9797E2F9C033
#define C0E5463D_72D1_43C1_9458_9797E2F9C033

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>


/**
 * \file ring_buffer.hpp
 *
 * @brief A simple ring-buffer with customizable behavior on overflow/underflow.
 */

namespace lf {

inline namespace ext {

/**
 * @brief A fixed capacity, power-of-two FILO ring-buffer with customizable behavior on overflow/underflow.
 */
template <std::default_initializable T, std::size_t N>
  requires (std::has_single_bit(N))
class ring_buffer {

  struct discard {
    LF_STATIC_CALL constexpr auto operator()(T const &) LF_STATIC_CONST noexcept -> bool { return false; }
  };

 public:
  /**
   * @brief Test whether the ring-buffer is empty.
   */
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return m_top == m_bottom; }

  /**
   * @brief Test whether the ring-buffer is full.
   */
  [[nodiscard]] constexpr auto full() const noexcept -> bool { return m_bottom - m_top == N; }

  /**
   * @brief Pushes a value to the ring-buffer.
   *
   * If the buffer is full then calls `when_full` with the value and returns false, otherwise returns true.
   * By default, `when_full` is a no-op.
   */
  template <std::invocable<T const &> F = discard>
  constexpr auto push(T const &val, F &&when_full = {}) noexcept(std::is_nothrow_invocable_v<F, T const &>) -> bool {
    if (full()) {
      std::invoke(std::forward<F>(when_full), val);
      return false;
    }
    store(m_bottom++, val);
    return true;
  }

  /**
   * @brief Pops (removes and returns) the last value pushed into the ring-buffer.
   *
   * If the buffer is empty calls `when_empty` and returns the result. By default, `when_empty` is a no-op that returns
   * a null `std::optional<T>`.
   */
  template <std::invocable F = impl::return_nullopt<T>>
    requires std::convertible_to<T &, std::invoke_result_t<F>>
  constexpr auto pop(F &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<F>) -> std::invoke_result_t<F> {
    if (empty()) {
      return std::invoke(std::forward<F>(when_empty));
    }
    return load(--m_bottom);
  }

 private:
  auto store(std::size_t index, T const &val) noexcept -> void {
    m_buff[index & mask] = val; // NOLINT
  }

  [[nodiscard]] auto load(std::size_t index) noexcept -> T & {
    return m_buff[(index & mask)]; // NOLINT
  }

  static constexpr std::size_t mask = N - 1;

  std::size_t m_top = 0;
  std::size_t m_bottom = 0;
  std::array<T, N> m_buff;
};

} // namespace ext

} // namespace lf

#endif /* C0E5463D_72D1_43C1_9458_9797E2F9C033 */


/**
 * @file contexts.hpp
 *
 * @brief A collection of `thread_context` implementations for different purposes.
 */

namespace lf::impl {

template <typename CRTP>
struct immediate_base {

  static void submit(intruded_h<CRTP> *ptr) { resume(unwrap(non_null(ptr))); }

  static void stack_push(async_stack *stack) {
    LF_LOG("stack_push");
    LF_ASSERT(stack);
    delete stack;
  }

  static auto stack_pop() -> async_stack * {
    LF_LOG("stack_pop");
    return new async_stack;
  }
};

// --------------------------------------------------------------------- //

/**
 * @brief The context type for the scheduler.
 */
class immediate_context : public immediate_base<immediate_context> {
 public:
  /**
   * @brief Returns `1` as this runs all tasks inline.
   *
   * \rst
   *
   * .. note::
   *
   *    As this is a static constexpr function the promise will transform all `fork -> call`.
   *
   * \endrst
   */
  static constexpr auto max_threads() noexcept -> std::size_t { return 1; }

  /**
   * @brief This should never be called due to the above.
   */
  auto task_pop() -> task_h<immediate_context> * {
    LF_ASSERT("false");
    return nullptr;
  }

  /**
   * @brief This should never be called due to the above.
   */
  void task_push(task_h<immediate_context> *) { LF_ASSERT("false"); }
};

static_assert(single_thread_context<immediate_context>);

// --------------------------------------------------------------------- //

/**
 * @brief An internal context type for testing purposes.
 *
 * This is essentially an immediate context with a task queue.
 */
class test_immediate_context : public immediate_base<test_immediate_context> {
 public:
  test_immediate_context() { m_tasks.reserve(1024); }

  // Deliberately not constexpr such that the promise will not transform all `fork -> call`.
  static auto max_threads() noexcept -> std::size_t { return 1; }

  /**
   * @brief Pops a task from the task queue.
   */
  auto task_pop() -> task_h<test_immediate_context> * {

    if (m_tasks.empty()) {
      return nullptr;
    }

    auto *last = m_tasks.back();
    m_tasks.pop_back();
    return last;
  }

  /**
   * @brief Pushes a task to the task queue.
   */
  void task_push(task_h<test_immediate_context> *task) { m_tasks.push_back(non_null(task)); }

 private:
  std::vector<task_h<test_immediate_context> *> m_tasks; // All non-null.
};

static_assert(thread_context<test_immediate_context>);
static_assert(!single_thread_context<test_immediate_context>);

// --------------------------------------------------------------------- //

/**
 * @brief A generic `thread_context` suitable for all `libforks` multi-threaded schedulers.
 *
 * This object does not manage worker_init/worker_finalize as it is intended
 * to be constructed/destructed by the master thread.
 */
template <typename CRTP>
class numa_worker_context : immovable<numa_worker_context<CRTP>> {

  static constexpr std::size_t k_buff = 16;
  static constexpr std::size_t k_steal_attempts = 32; ///< Attempts per target.

  using task_t = task_h<CRTP>;
  using submit_t = submit_h<CRTP>;
  using intruded_t = intruded_h<CRTP>;

  deque<task_t *> m_tasks;                     ///< Our public task queue, all non-null.
  deque<async_stack *> m_stacks;               ///< Our public stack queue, all non-null.
  intrusive_list<submit_t *> m_submit;         ///< The public submission queue, all non-null.
  ring_buffer<async_stack *, k_buff> m_buffer; ///< Our private stack buffer, all non-null.

  xoshiro m_rng;                            ///< Our personal PRNG.
  std::size_t m_max_threads;                ///< The total max parallelism available.
  std::vector<std::vector<CRTP *>> m_neigh; ///< Our view of the NUMA topology.

  template <typename T>
  static constexpr auto null_for = []() LF_STATIC_CALL noexcept -> T * {
    return nullptr;
  };

 public:
  numa_worker_context(std::size_t n, xoshiro const &rng) : m_rng(rng), m_max_threads(n) {
    for (std::size_t i = 0; i < k_buff / 2; ++i) {
      m_buffer.push(new async_stack);
    }
  }

  /**
   * @brief Initialize the context with the given topology and bind it to a thread.
   *
   * This is separate from construction as the master thread will need to construct
   * the contexts before they can form a reference to them, this must be called by the
   * worker thread.
   *
   * The context will store __raw__ pointers to the other contexts in the topology, this is
   * to ensure no circular references are formed.
   */
  void init_numa_and_bind(numa_topology::numa_node<CRTP> const &topo) {

    LF_ASSERT(topo.neighbors.front().front().get() == this);
    topo.bind();

    // Skip the first one as it is just us.
    for (std::size_t i = 1; i < topo.neighbors.size(); ++i) {
      m_neigh.push_back(map(topo.neighbors[i], [](std::shared_ptr<CRTP> const &context) {
        // We must use regular pointers to avoid circular references.
        return context.get();
      }));
    }
  }

  /**
   * @brief Fetch a linked-list of the submitted tasks.
   */
  auto try_get_submitted() noexcept -> intruded_t * { return m_submit.try_pop_all(); }

  /**
   * @brief Try to steal a task from one of our friends, returns `nullptr` if we failed.
   */
  auto try_steal() noexcept -> task_t * {

    for (auto &&friends : m_neigh) {

      for (std::size_t i = 0; i < k_steal_attempts; ++i) {

        std::shuffle(friends.begin(), friends.end(), m_rng);

        for (CRTP *context : friends) {

          auto [err, task] = context->m_tasks.steal();

          switch (err) {
            case lf::err::none:
              LF_LOG("Stole task from {}", (void *)context);
              return task;

            case lf::err::lost:
              // We don't retry here as we don't want to cause contention
              // and we have multiple steal attempts anyway.
            case lf::err::empty:
              continue;

            default:
              LF_ASSERT(false);
          }
        }
      }
    }

    return nullptr;
  }

  ~numa_worker_context() noexcept {
    //
    LF_ASSERT_NO_ASSUME(m_tasks.empty());

    while (auto *stack = m_buffer.pop(null_for<async_stack>)) {
      delete stack;
    }

    while (auto *stack = m_stacks.pop(null_for<async_stack>)) {
      delete stack;
    }
  }

  // ------------- To satisfy `thread_context` ------------- //

  auto max_threads() const noexcept -> std::size_t { return m_max_threads; }

  auto submit(intruded_t *node) noexcept -> void { m_submit.push(non_null(node)); }

  auto stack_pop() -> async_stack * {

    if (auto *stack = m_buffer.pop(null_for<async_stack>)) {
      LF_LOG("stack_pop() using local-buffered stack");
      return stack;
    }

    if (auto *stack = m_stacks.pop(null_for<async_stack>)) {
      LF_LOG("stack_pop() using public-buffered stack");
      return stack;
    }

    for (auto &&friends : m_neigh) {

      std::shuffle(friends.begin(), friends.end(), m_rng);

      for (CRTP *context : friends) {

      retry:
        auto [err, stack] = context->m_stacks.steal();

        switch (err) {
          case lf::err::none:
            LF_LOG("stack_pop() stole from {}", (void *)context);
            return stack;
          case lf::err::lost:
            // We retry (even if it may cause contention) to try and avoid allocating.
            goto retry;
          case lf::err::empty:
            break;
          default:
            LF_ASSERT(false);
        }
      }
    }

    LF_LOG("stack_pop() allocating");

    return new async_stack;
  }

  void stack_push(async_stack *stack) {
    m_buffer.push(non_null(stack), [&](async_stack *extra_stack) noexcept {
      LF_LOG("Local stack buffer overflows");
      m_stacks.push(extra_stack);
    });
  }

  auto task_pop() noexcept -> task_t * { return m_tasks.pop(null_for<task_t>); }

  void task_push(task_t *task) { m_tasks.push(non_null(task)); }
};

namespace detail::static_test {

struct test_context : numa_worker_context<test_context> {};

static_assert(thread_context<test_context>);
static_assert(!single_thread_context<test_context>);

} // namespace detail::static_test

} // namespace lf::impl

#endif /* C1B42944_8E33_4F6B_BAD6_5FB687F6C737 */


/**
 * @file busy.hpp
 *
 * @brief A work-stealing thread pool where all the threads spin when idle.
 */

namespace lf {

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 * Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class busy_pool {
 public:
  struct context_type : impl::numa_worker_context<context_type> {
    using numa_worker_context::numa_worker_context;
  };

 private:
  xoshiro m_rng{seed, std::random_device{}};

  std::vector<std::shared_ptr<context_type>> m_contexts;
  std::vector<std::thread> m_workers;
  std::unique_ptr<std::atomic_flag> m_stop = std::make_unique<std::atomic_flag>();

  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {

    LF_LOG("Requesting a stop");
    // Set conditions for workers to stop
    m_stop->test_and_set(std::memory_order_release);

    for (auto &worker : m_workers) {
      worker.join();
    }
  }

  static auto work(numa_topology::numa_node<context_type> node, std::atomic_flag const &stop_requested) {

    std::shared_ptr my_context = node.neighbors.front().front();

    worker_init(my_context.get());

    impl::defer at_exit = [&]() noexcept {
      worker_finalize(my_context.get());
    };

    my_context->init_numa_and_bind(node);

    while (!stop_requested.test(std::memory_order_acquire)) {

      for_each(my_context->try_get_submitted(), [](submit_h<context_type> *submitted) LF_STATIC_CALL noexcept {
        resume(submitted);
      });

      if (auto *task = my_context->try_steal()) {
        resume(task);
      }
    };
  }

 public:
  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) {

    for (std::size_t i = 0; i < n; ++i) {
      m_contexts.push_back(std::make_shared<context_type>(n, m_rng));
      m_rng.long_jump();
    }

    LF_ASSERT_NO_ASSUME(!m_stop->test(std::memory_order_acquire));

    std::vector nodes = numa_topology{}.distribute(m_contexts);

    // clang-format off

    LF_TRY {
      for (auto &&node : nodes) {
        m_workers.emplace_back(work, std::move(node), std::cref(*m_stop));
      }
    } LF_CATCH_ALL {
      clean_up();
      LF_RETHROW;
    }

    // clang-format on
  }

  ~busy_pool() noexcept { clean_up(); }

  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(intruded_h<context_type> *node) noexcept {
    std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);
    m_contexts[dist(m_rng)]->submit(node);
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
 * This file has been adapted from: ``https://github.com/facebook/folly/blob/main/folly/experimental/EventCount.h``
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

static constexpr std::memory_order acquire = std::memory_order_acquire;
static constexpr std::memory_order acq_rel = std::memory_order_acq_rel;
static constexpr std::memory_order release = std::memory_order_release;

static constexpr std::uint64_t k_thieve = 1;
static constexpr std::uint64_t k_active = k_thieve << 32U;

static constexpr std::uint64_t k_thieve_mask = (k_active - 1);
static constexpr std::uint64_t k_active_mask = ~k_thieve_mask;

/**
 * @brief Need to overload submit to add notifications.
 */
class lazy_context : public numa_worker_context<lazy_context> {
 public:
  struct remote_atomics {
    /**
     * Effect:
     *
     * T <- T - 1
     * S <- S
     * A <- A + 1
     *
     * A is now guaranteed to be greater than 0, if we were the last thief we try to wake someone.
     *
     * Then we do the task.
     *
     * Once we are done we perform:
     *
     * T <- T + 1
     * S <- S
     * A <- A - 1
     *
     * This never invalidates the invariant.
     *
     *
     * Overall effect: thief->active, do the work, active->thief.
     */
    template <typename Handle>
      requires std::same_as<Handle, task_h<lazy_context>> || std::same_as<Handle, intruded_h<lazy_context>>
    void thief_round_trip(Handle *handle) noexcept {

      auto prev_thieves = dual_count.fetch_add(k_active - k_thieve, acq_rel) & k_thieve_mask;

      if (prev_thieves == 1) {
        LF_LOG("The last thief wakes someone up");
        notifier.notify_one();
      }

      if constexpr (std::same_as<Handle, intruded_h<lazy_context>>) {
        for_each(handle, [](submit_h<lazy_context> *submitted) LF_STATIC_CALL noexcept {
          resume(submitted);
        });
      } else {
        resume(handle);
      }

      dual_count.fetch_sub(k_active - k_thieve, acq_rel);
    }

    alignas(k_cache_line) std::atomic_uint64_t dual_count = 0;
    alignas(k_cache_line) std::atomic_flag stop;
    alignas(k_cache_line) event_count notifier;
  };

  // ---------------------------------------------------------------------- //

  /**
   * @brief Submissions to the `lazy_pool` are very noisy (everyone wakes up).
   */
  auto submit(intruded_h<lazy_context> *node) noexcept -> void {
    numa_worker_context::submit(node);
    m_atomics->notifier.notify_all();
  }

  // ---------------------------------------------------------------------- //

  lazy_context(std::size_t n, xoshiro &rng, std::shared_ptr<remote_atomics> atomics)
      : numa_worker_context{n, rng},
        m_atomics(std::move(atomics)) {}

  static auto work(numa_topology::numa_node<lazy_context> node) {

    // ---- Initialization ---- //

    std::shared_ptr my_context = node.neighbors.front().front();

    LF_ASSERT(my_context.get());

    worker_init(my_context.get());

    impl::defer at_exit = [&]() noexcept {
      worker_finalize(my_context.get());
    };

    my_context->init_numa_and_bind(node);

    /**
     * Invariant we want to uphold:
     *
     *  If there is an active task their is always: [a thief] OR [no sleeping].
     *
     * Let:
     *  T = number of thieves
     *  S = number of sleeping threads
     *  A = number of active threads
     *
     * Invariant: *** if (A > 0) then (T >= 1 OR S == 0) ***
     *
     * Lemma 1: Promoting an S -> T guarantees that the invariant is upheld.
     *
     * Proof 1:
     *  Case S != 0, then T -> T + 1, hence T > 0 hence invariant maintained.
     *  Case S == 0, then invariant is already maintained.
     */

  wake_up:
    /**
     * Invariant maintained by Lemma 1 regardless if this is a wake up (S <- S - 1) or join (S <- S).
     */
    my_context->m_atomics->dual_count.fetch_add(k_thieve, release);

  continue_as_thief:
    /**
     * First we handle the fast path (work to do) before touching the notifier.
     */
    if (auto *submission = my_context->try_get_submitted()) {
      my_context->m_atomics->thief_round_trip(submission);
      goto continue_as_thief;
    }
    if (auto *stolen = my_context->try_steal()) {
      my_context->m_atomics->thief_round_trip(stolen);
      goto continue_as_thief;
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

    auto key = my_context->m_atomics->notifier.prepare_wait();

    if (auto *submission = my_context->try_get_submitted()) {
      // Check our private **before** `stop`.
      my_context->m_atomics->notifier.cancel_wait();
      my_context->m_atomics->thief_round_trip(submission);
      goto continue_as_thief;
    }

    if (my_context->m_atomics->stop.test(acquire)) {
      // A stop has been requested, we will honor it under the assumption
      // that the requester has ensured that everyone is done. We cannot check
      // this i.e it is possible a thread that just signaled the master thread
      // is still `active` but act stalled.
      my_context->m_atomics->notifier.cancel_wait();
      // We leave a "ghost thief" here e.g. don't bother to reduce the counter,
      // This is fine because no-one can sleep now that the stop flag is set.
      return;
    }

    /**
     * Try:
     *
     * T <- T - 1
     * S <- S + 1
     * A <- A
     *
     * If new T == 0 and A > 0 then wake self immediately i.e:
     *
     * T <- T + 1
     * S <- S - 1
     * A <- A
     *
     * If we return true then we are safe to sleep, otherwise we must stay awake.
     */

    auto prev_dual = my_context->m_atomics->dual_count.fetch_sub(k_thieve, acq_rel);

    // We are now registered as a sleeping thread and may have broken the invariant.

    auto prev_thieves = prev_dual & k_thieve_mask;
    auto prev_actives = prev_dual & k_active_mask; // Again only need 0 or non-zero.

    if (prev_thieves == 1 && prev_actives != 0) {
      // Restore the invariant.
      goto wake_up;
    }

    LF_LOG("Goes to sleep");

    // We are safe to sleep.
    my_context->m_atomics->notifier.wait(key);
    // Note, this could be a spurious wakeup, that doesn't matter because we will just loop around.
    goto wake_up;
  }

 private:
  std::shared_ptr<remote_atomics> m_atomics;
};

} // namespace impl

/**
 * @brief A scheduler based on a [An Efficient Work-Stealing Scheduler for Task Dependency
 * Graph](https://doi.org/10.1109/icpads51040.2020.00018)
 *
 * This pool sleeps workers which cannot find any work, as such it should be the default choice for most
 * use cases. Additionally (if an installation of `hwloc` was found) this pool is NUMA aware.
 */
class lazy_pool {
 public:
  using context_type = impl::lazy_context;

 private:
  using remote = typename context_type::remote_atomics;

  std::shared_ptr<remote> m_atomics = std::make_shared<remote>();
  xoshiro m_rng{seed, std::random_device{}};
  std::uniform_int_distribution<std::size_t> m_dist;
  std::vector<std::shared_ptr<context_type>> m_contexts;
  std::vector<std::thread> m_workers;

  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {
    LF_LOG("Requesting a stop");

    // Set conditions for workers to stop.
    m_atomics->stop.test_and_set(std::memory_order_release);
    m_atomics->notifier.notify_all();

    for (auto &worker : m_workers) {
      worker.join();
    }
  }

 public:
  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(intruded_h<context_type> *node) noexcept { m_contexts[m_dist(m_rng)]->submit(node); }

  /**
   * @brief Construct a new lazy_pool object and `n` worker threads.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit lazy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_dist{0, n - 1} {

    for (std::size_t i = 0; i < n; ++i) {
      m_contexts.push_back(std::make_shared<context_type>(n, m_rng, m_atomics));
      m_rng.long_jump();
    }

    std::vector nodes = numa_topology{}.distribute(m_contexts);

    // clang-format off

    LF_TRY {
      for (auto &&node : nodes) {
        m_workers.emplace_back(context_type::work, std::move(node));
      }
    } LF_CATCH_ALL {
      clean_up();
      LF_RETHROW;
    }

    // clang-format on
  }

  ~lazy_pool() noexcept { clean_up(); }
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

#include <vector>



/**
 * @file unit_pool.hpp
 *
 * @brief A scheduler that runs all tasks inline on the current thread.
 */

namespace lf {

namespace impl {

template <thread_context Context>
class unit_pool_impl : impl::immovable<unit_pool_impl<Context>> {
 public:
  using context_type = Context;

  static void schedule(intruded_h<context_type> *ptr) { context_type::submit(ptr); }

  unit_pool_impl() { worker_init(&m_context); }

  ~unit_pool_impl() { worker_finalize(&m_context); }

 private:
  [[no_unique_address]] context_type m_context;
};

} // namespace impl

inline namespace ext {

/**
 * @brief A scheduler that runs all tasks inline on the current thread and keeps an internal stack.
 *
 * This is exposed/intended for testing, using this thread pool is equivalent to
 * using a `busy_pool` with a single thread. It is different from `unit_pool` in that
 * it explicitly disables the `fork` -> `call` optimisation.
 */
using debug_pool = impl::unit_pool_impl<impl::test_immediate_context>;

static_assert(scheduler<debug_pool>);

} // namespace ext

/**
 * @brief A scheduler that runs all tasks inline on the current thread.
 *
 * This is useful for testing/debugging/benchmarking.
 */
using unit_pool = impl::unit_pool_impl<impl::immediate_context>;

static_assert(scheduler<unit_pool>);

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
