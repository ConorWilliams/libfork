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

#include "libfork/macro.hpp"

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
 * @brief A inline namespace that wraps core functionality.
 *
 * This is the namespace that contains the user facing API of ``libfork``.
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
 * This is exposed for internal documentation however it is not part of the public facing API. No entities wrapped in this
 * namespace should be used as their API's are not stable.
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
 * @brief Forwards to ``!std::is_reference_v<T>``.
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
