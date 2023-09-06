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
 * @brief Shorthand for `std::numeric_limits<std::uint32_t>::max()`.
 */
static constexpr std::uint32_t k_u32_max = std::numeric_limits<std::uint32_t>::max();

inline constexpr std::size_t k_kibibyte = 1024 * 1;          // NOLINT
inline constexpr std::size_t k_mebibyte = 1024 * k_kibibyte; //

/**
 * @brief An empty type that is not copyable or movable.
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
 * @brief Get an integral representation of a pointer.
 */
template <unqualified T>
auto as_integer(T *ptr) -> std::uintptr_t {
  return std::bit_cast<std::uintptr_t>(ptr);
}

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
 * @brief An (unchecked, transparent) annotation to indicate that a pointer is non-null.
 */
template <typename T>
  requires std::is_pointer_v<T>
using non_null = T;

/**
 * @brief An (unchecked, transparent) annotation to indicate that a pointer is owning.
 */
template <typename T>
  requires std::is_pointer_v<T>
using owner = T;

#endif /* DF63D333_F8C0_4BBA_97E1_32A78466B8B7 */
