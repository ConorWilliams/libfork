#ifndef D66428B1_3B80_45ED_A7C2_6368A0903810
#define D66428B1_3B80_45ED_A7C2_6368A0903810

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>

#include "libfork/coroutine.hpp"
#include "libfork/exception.hpp"
#include "libfork/macro.hpp"
#include "libfork/stack.hpp"

/**
 * @file promise_base.hpp
 *
 * @brief Provides the promise_type's common donominator.
 */

namespace lf {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 */
enum class tag {
  root, ///< This coroutone is a root task (allocated on heap) fro an ``lf::sync_wait``.
  call, ///< Non root task (on a virtual stack) from an ``lf::call``.
  fork, ///< Non root task (on a virtual stack) from an ``lf::fork``.
};

namespace detail {

// -------------- Control block definition -------------- //

static constexpr std::int32_t k_imax = std::numeric_limits<std::int32_t>::max();

template <typename T>
struct root_block_t {
  exception_packet exception{};
  std::binary_semaphore semaphore{0};
  std::optional<T> result{};
};

template <>
struct root_block_t<void> {
  exception_packet exception{};
  std::binary_semaphore semaphore{0};
};

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&root_block_t<long>::m_exception));
static_assert(std::is_pointer_interconvertible_with_class(&root_block_t<void>::m_exception));
#endif

class promise_base {
public:
  // Full declaration below, needs concept first
  class handle_t;

  constexpr void set_ret_address(void *ret) noexcept {
    m_return_address = ret;
  }
  constexpr void set_parent(stdexp::coroutine_handle<promise_base> parent) noexcept {
    m_parent = parent;
  }

  [[nodiscard]] constexpr auto has_parent() const noexcept -> bool {
    return static_cast<bool>(m_parent);
  }

  // Checked access
  [[nodiscard]] constexpr auto parent() const noexcept -> stdexp::coroutine_handle<promise_base> {
    LIBFORK_ASSERT(m_parent);
    return m_parent;
  }

  [[nodiscard]] constexpr auto ret_address() const noexcept -> void * {
    LIBFORK_ASSERT(m_return_address);
    return m_return_address;
  }

  [[nodiscard]] constexpr auto steals() noexcept -> std::int32_t & {
    return m_steal;
  }

  [[nodiscard]] constexpr auto joins() noexcept -> std::atomic_int32_t & {
    return m_join;
  }

  constexpr void reset() noexcept {
    // This is called when taking ownership of a task at a join point.
    LIBFORK_ASSERT(m_steal != 0);
    m_steal = 0;
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory syncronisation.
    static_assert(std::is_trivially_destructible_v<std::atomic_int32_t>);

    std::construct_at(&m_join, k_imax);
  }

private:
  stdexp::coroutine_handle<promise_base> m_parent = {}; ///< Parent task (roots don't have one).
  void *m_return_address = nullptr;                     ///< root_block * || T *
  std::int32_t m_steal = 0;                             ///< Number of steals.
  std::atomic_int32_t m_join = k_imax;                  ///< Number of children joined (obfuscated).
};

// -------------- promise_base -------------- //

} // namespace detail

/**
 * @brief A handle to a task with a resume() member function.
 */
using task_handle = detail::promise_base::handle_t;

/**
 * @brief A concept which requires a type to define a ``stack_type`` which must be a specialization of ``lf::virtual_stack``.
 */
template <typename T>
concept defines_stack = requires { typename T::stack_type; } && detail::is_virtual_stack<typename T::stack_type>;

// clang-format off

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of virtual-stacks and a LIFO stack of tasks.
 * The stack of virtual-stacks is expected to never be empty, it should always
 * be able to return an empty virtual-stack.
 *
 * \rst
 *
 * Specifically this requires:
 *
 * .. code::
 *
 *      typename Context::stack_type;
 *
 *      requires // Context::stack_type is a specialization of lf::virtual_stack //;
 *
 *      // Access the thread_local context.
 *      { Context::context() } -> std::same_as<Context &>;
 *
 *      // Check the maximum parallelism.
 *      { Context::max_threads() } -> std::same_as<std::size_t>;
 *
 *      // Access the top stack.
 *      { ctx.stack_top() } -> std::convertible_to<typename Context::stack_type::handle>;
 *      // Remove the top stack.
 *      { ctx.stack_pop() };
 *      // Insert stack at the top.
 *      { ctx.stack_push(stack) };
 *
 *      // Remove and return the top task.
 *      { ctx.task_pop() } -> std::convertible_to<std::optional<task_handle>>;
 *      // Insert task at the top.
 *      { ctx.task_push(handle) };
 *
 *
 * \endrst
 */
template <typename Context>
concept thread_context = defines_stack<Context> && requires(Context ctx, typename Context::stack_type::handle stack, task_handle handle) {
  { Context::context() } -> std::same_as<Context &>; 

  { ctx.max_threads() } -> std::same_as<std::size_t>; 

  { ctx.stack_top() } -> std::convertible_to<typename Context::stack_type::handle>; 
  { ctx.stack_pop() };                                                              
  { ctx.stack_push(stack) };                                                        

  { ctx.task_pop() } -> std::convertible_to<std::optional<task_handle>>; 
  { ctx.task_push(handle) };                                             
};

// clang-format on

// -------------- Define forward decls -------------- //

class detail::promise_base::handle_t : private stdexp::coroutine_handle<promise_base> {
public:
  handle_t() = default; ///< To make us a trivial type.

  void resume() noexcept {
    LIBFORK_LOG("Call to resume on stolen task");
    LIBFORK_ASSERT(*this);

    stdexp::coroutine_handle<promise_base>::promise().m_steal += 1;
    stdexp::coroutine_handle<promise_base>::resume();
  }

private:
  template <typename T, thread_context Context, tag Tag>
  friend struct promise_type;

  explicit handle_t(stdexp::coroutine_handle<promise_base> handle) : stdexp::coroutine_handle<promise_base>{handle} {}
};

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
