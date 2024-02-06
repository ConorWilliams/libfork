#ifndef A75DC3F0_D0C3_4669_A901_0B22556C873C
#define A75DC3F0_D0C3_4669_A901_0B22556C873C

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits>

#include "libfork/core/impl/utility.hpp"

/**
 * @file tag.hpp
 *
 * @brief Libfork's dispatch tags.
 */

namespace lf {

inline namespace core {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 *
 * You can inspect the first argument of an async function to determine the tag.
 */
enum class tag {
  /**
   * @brief This coroutine is a root task from an ``lf::sync_wait``.
   */
  root,
  /**
   * @brief Non root task from an ``lf::call``, completes synchronously.
   */
  call,
  /**
   * @brief Non root task from an ``lf::fork``, completes asynchronously.
   */
  fork,
};

/**
 * @brief Modifier's for the dispatch tag, these do not effect the child's promise, only the awaitable.
 *
 * See `lf::core::dispatch` for more information and uses, these are low-level and most users should not need
 * to use them. We use a namespace + types rather than an enumeration to allow for type-concepts.
 */
namespace modifier {
/**
 * @brief No modification to the dispatch category.
 */
struct none {};
/**
 * @brief The dispatch is `fork`, reports if the fork completed synchronously.
 */
struct sync {};
/**
 * @brief The dispatch is a `fork` outside a fork-join scope, reports if the fork completed synchronously.
 */
struct sync_outside {};
/**
 * @brief The dispatch is a `call`, the awaitable will throw eagerly.
 */
struct eager_throw {};
/**
 * @brief The dispatch is a `call` outside a fork-join scope, the awaitable will throw eagerly.
 */
struct eager_throw_outside {};

} // namespace modifier

} // namespace core

namespace impl::detail {

template <typename Mod, tag T>
struct valid_modifier_impl : std::false_type {
  static_assert(impl::always_false<Mod>, "Mod is not a valid modifier for tag T!");
};

template <tag T>
struct valid_modifier_impl<modifier::none, T> : std::true_type {};

template <>
struct valid_modifier_impl<modifier::sync, tag::fork> : std::true_type {};

template <>
struct valid_modifier_impl<modifier::sync_outside, tag::fork> : std::true_type {};

// TODO: in theory it is possible to extend eager to fork but you may as well just use sync[_outside]?

template <>
struct valid_modifier_impl<modifier::eager_throw, tag::call> : std::true_type {};

template <>
struct valid_modifier_impl<modifier::eager_throw_outside, tag::call> : std::true_type {};

} // namespace impl::detail

inline namespace core {

/**
 * @brief Test if a type is a valid modifier for a tag.
 */
template <typename T, tag Tag>
concept modifier_for = impl::detail::valid_modifier_impl<T, Tag>::value;

} // namespace core

namespace impl {

/**
 * @brief An enumerator describing a statement's location wrt to a fork-join scope.
 */
enum class region {
  /**
   * @brief Unknown location wrt to a fork-join scope.
   */
  unknown,
  /**
   * @brief Outside a fork-join scope.
   */
  outside,
  /**
   * @brief Inside a fork-join scope
   */
  inside,
  /**
   * @brief First fork statement in a fork-join scope.
   */
  opening_fork,
};

} // namespace impl

} // namespace lf

#endif /* A75DC3F0_D0C3_4669_A901_0B22556C873C */
