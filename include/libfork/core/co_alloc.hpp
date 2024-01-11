#ifndef A951FB73_0FCF_4B7C_A997_42B7E87D21CB
#define A951FB73_0FCF_4B7C_A997_42B7E87D21CB
// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts> // for default_initializable
#include <cstddef>  // for size_t
#include <memory>
#include <span> // for dynamic_extent, span
#include <type_traits>
#include <utility>

#include "libfork/core/ext/tls.hpp"
#include "libfork/core/impl/frame.hpp"
#include "libfork/core/impl/utility.hpp" // for k_new_align

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

} // namespace impl

inline namespace core {

/**
 * @brief The result of `co_await`ing the result of ``lf::core::co_new``.
 *
 * A raii wrapper around a ``std::span`` pointing to the memory allocated on the stack.
 * This type can be destructured into a ``std::span``/pointer to the allocated memory.
 */
template <co_allocable T, std::size_t Extent>
class stack_allocated : impl::immovable<stack_allocated<T, Extent>> {
 public:
  /**
   * @brief Construct a new co allocated object.
   */
  stack_allocated(impl::frame *frame, std::span<T, Extent> span) noexcept : m_span{span}, m_frame{frame} {}

  /**
   * @brief Get a span/pointer, depending on the extent, to the allocated memory.
   */
  template <std::size_t I>
    requires (I == 0)
  auto get() const noexcept -> std::conditional_t<Extent == 1, T *, std::span<T, Extent>> {
    if constexpr (Extent == 1) {
      return m_span.data();
    } else {
      return m_span;
    }
  }

  /**
   * @brief For consistent handling in generic code.
   */
  auto span() const noexcept -> std::span<T, Extent> { return m_span; }

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
  std::span<T, Extent> m_span;
};

} // namespace core

} // namespace lf

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

template <lf::co_allocable T, std::size_t Extent>
struct std::tuple_size<lf::stack_allocated<T, Extent>> : std::integral_constant<std::size_t, 1> {};

template <lf::co_allocable T, std::size_t Extent>
struct std::tuple_element<0, lf::stack_allocated<T, Extent>> : std::type_identity<std::span<T, Extent>> {};

template <lf::co_allocable T>
struct std::tuple_element<0, lf::stack_allocated<T, 1>> : std::type_identity<T *> {};

#endif

namespace lf {

inline namespace core {

/**
 * @brief A function which returns an awaitable (in the context of an ``lf::task``) which triggers allocation
 * on a worker's stack.
 *
 * Upon ``co_await``ing the result of this function an ``lf::stack_allocated`` object is returned.
 *
 *
 * \rst
 *
 * .. warning::
 *    This must be called __outside__ of a fork-join scope and is an expert only feature!
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
 * on a worker's stack.
 *
 * Upon ``co_await``ing the result of this function an ``lf::stack_allocated`` object is returned.
 *
 *
 * \rst
 *
 * .. warning::
 *    This must be called __outside__ of a fork-join scope and is an expert only feature!
 *
 * \endrst
 *
 */
template <co_allocable T, std::size_t Extent = 1>
  requires (Extent != std::dynamic_extent)
inline auto co_new() -> impl::co_new_t<T, Extent> {
  return {};
}

} // namespace core

} // namespace lf

#endif /* A951FB73_0FCF_4B7C_A997_42B7E87D21CB */
