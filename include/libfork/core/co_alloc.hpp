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

#include "libfork/core/ext/tls.hpp"

#include "libfork/core/impl/frame.hpp"
#include "libfork/core/impl/utility.hpp"

/**
 * @file co_alloc.hpp
 *
 * @brief Expert-only utilities to interact with a coroutines fibre.
 */

namespace lf {

template <typename T>
concept co_allocable = std::default_initializable<T> && alignof(T) <= impl::k_new_align;

namespace impl {

/**
 * @brief An awaitable (in the context of an ``lf::task``) which triggers stack allocation.
 */
template <co_allocable T, std::size_t Extent>
struct [[nodiscard("This object should be co_awaited")]] co_new_t {
  static constexpr std::size_t count = Extent; ///< The number of elements to allocate.
};

template <co_allocable T>
struct [[nodiscard("This object should be co_awaited")]] co_new_t<T, std::dynamic_extent> {
  std::size_t count; ///< The number of elements to allocate.
};

template <co_allocable T, std::size_t Extent>
struct [[nodiscard("This object should be co_awaited")]] co_delete_t : std::span<T, Extent> {};

} // namespace impl

/**
 * @brief A function which returns an awaitable (in the context of an ``lf::task``) which triggers allocation
 * on a fibre's stack.
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
 * on a fibre's stack.
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

} // namespace lf

#endif /* A951FB73_0FCF_4B7C_A997_42B7E87D21CB */
