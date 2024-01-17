#ifndef A951FB73_0FCF_4B7C_A997_42B7E87D21CB
#define A951FB73_0FCF_4B7C_A997_42B7E87D21CB
// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>       // for tuple_element, tuple_size
#include <concepts>    // for default_initializable
#include <cstddef>     // for size_t
#include <memory>      // for destroy
#include <span>        // for span
#include <type_traits> // for integral_constant, type_ide...
#include <utility>

#include "libfork/core/ext/tls.hpp"      // for stack
#include "libfork/core/impl/frame.hpp"   // for frame
#include "libfork/core/impl/stack.hpp"   // for stack
#include "libfork/core/impl/utility.hpp" // for immovable, k_new_align

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
template <co_allocable T>
struct [[nodiscard("This object should be co_awaited")]] co_new_t {
  std::size_t count; ///< The number of elements to allocate.
};

} // namespace impl

inline namespace core {

/**
 * @brief The result of `co_await`ing the result of ``lf::core::co_new``.
 *
 * A raii wrapper around a ``std::span`` pointing to the memory allocated on the stack.
 * This type can be destructured into a ``std::span`` to the allocated memory.
 */
template <co_allocable T>
class stack_allocated : impl::immovable<stack_allocated<T>> {
 public:
  /**
   * @brief Construct a new co allocated object.
   */
  stack_allocated(impl::frame *frame, std::span<T> span) noexcept : m_frame{frame}, m_span{span} {}

  /**
   * @brief Get a span over the allocated memory.
   */
  template <std::size_t I>
    requires (I == 0)
  auto get() noexcept -> std::span<T> {
    return m_span;
  }

  /**
   * @brief Get a span over the allocated memory.
   */
  template <std::size_t I>
    requires (I == 0)
  auto get() const noexcept -> std::span<T const> {
    return m_span;
  }

  /**
   * @brief Destroys objects and releases the memory.
   */
  ~stack_allocated() noexcept {
    std::ranges::destroy(m_span);
    auto *stack = impl::tls::stack();
    stack->deallocate(m_span.data());
    m_frame->reset_stacklet(stack->top());
  }

 private:
  impl::frame *m_frame;
  std::span<T> m_span;
};

} // namespace core

} // namespace lf

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

template <lf::co_allocable T>
struct std::tuple_size<lf::stack_allocated<T>> : std::integral_constant<std::size_t, 1> {};

template <lf::co_allocable T>
struct std::tuple_size<lf::stack_allocated<T> const> : std::integral_constant<std::size_t, 1> {};

template <lf::co_allocable T>
struct std::tuple_element<0, lf::stack_allocated<T>> : std::type_identity<std::span<T>> {};

template <lf::co_allocable T>
struct std::tuple_element<0, lf::stack_allocated<T> const> : std::type_identity<std::span<T const>> {};

#endif

namespace lf {

inline namespace core {

/**
 * @brief A function which returns an awaitable which triggers allocation on a worker's stack.
 *
 * Upon ``co_await``ing the result of this function an ``lf::stack_allocated`` object is returned.
 *
 * \rst
 *
 * .. warning::
 *    This must be called __outside__ of a fork-join scope and is an expert only feature!
 *
 * \endrst
 *
 */
template <co_allocable T>
inline auto co_new(std::size_t count) -> impl::co_new_t<T> {
  return impl::co_new_t<T>{count};
}

} // namespace core

} // namespace lf

#endif /* A951FB73_0FCF_4B7C_A997_42B7E87D21CB */
