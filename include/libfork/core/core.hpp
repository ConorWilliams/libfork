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

#include "libfork/core/coroutine.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

/**
 * @file core.hpp
 *
 * @brief Provides the promise_type's common denominator.
 */

namespace lf {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 */
enum class tag {
  root, ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call, ///< Non root task (on a virtual stack) from an ``lf::call``.
  fork, ///< Non root task (on a virtual stack) from an ``lf::fork``.
};

namespace detail {

// -------------- Control block definition -------------- //

template <typename T>
struct root_block_t;

template <>
struct root_block_t<void> : immovable<root_block_t<void>> {
  std::binary_semaphore semaphore{0};
};

template <typename T>
struct root_block_t : root_block_t<void> {

  std::optional<T> result{};

  template <typename U>
    requires std::constructible_from<std::optional<T>, U>
  constexpr auto operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) -> root_block_t & {

    LF_LOG("Root task assigns");

    LF_ASSERT(!result.has_value());

    result.emplace(std::forward<U>(expr));

    return *this;
  }
};

// ----------------------------------------------- //

/* In theory if the compiler coroutine frame looks like this:

struct coroutine_frame {
  ... // maybe excess-alignment padding here when coro frame alignment > 2 * sizeof (ptr).
  void (*resume_fn)();
  void (*destroy_fn)();
  promise_type promise;
  ... // Other needed variables
};

If we know that there is no excess-alignment padding, then we can compress
all of this into just the parent pointer and just pass around a pointer to
the coroutine_frame.
*/

/**
 * @brief A base class for all promise_types.
 *
 * Stores a partially type-erased pointer to the parent task, as
 * well as fork/join specific counters.
 */
class control_block_t : immovable<control_block_t> {
public:
  // Full declaration below, needs concept first
  class handle_t;

  // Either a T* for fork/call or root_block_t *
  constexpr void set_ret_address(void *ret) noexcept {
    m_return_address = ret;
  }
  constexpr void set_parent(stdx::coroutine_handle<control_block_t> parent) noexcept {
    m_parent = parent;
  }

  [[nodiscard]] constexpr auto has_parent() const noexcept -> bool {
    return static_cast<bool>(m_parent);
  }

  // Checked access
  [[nodiscard]] constexpr auto parent() const noexcept -> stdx::coroutine_handle<control_block_t> {
    LF_ASSERT(has_parent());
    return m_parent;
  }

  [[nodiscard]] constexpr auto ret_address() const noexcept -> void * {
    LF_ASSERT(m_return_address);
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
    LF_ASSERT(m_steal != 0);

    m_steal = 0;
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    static_assert(std::is_trivially_destructible_v<std::atomic_int32_t>);

    std::construct_at(&m_join, k_imax);
  }

  // Increase the debug counter
  constexpr void debug_inc() noexcept {
#ifndef NDEBUG
    LF_ASSERT(m_debug_count < std::numeric_limits<std::int32_t>::max());
    ++m_debug_count;
#endif
  }
  // Fetch the debug count
  [[nodiscard]] constexpr auto debug_count() const noexcept -> std::int64_t {
#ifndef NDEBUG
    return m_debug_count;
#else
    return 0;
#endif
  }
  // Reset the debug counter
  constexpr void debug_reset() noexcept {
#ifndef NDEBUG
    m_debug_count = 0;
#endif
  }

private:
  std::atomic_int32_t m_join = k_imax; ///< Number of children joined (obfuscated).
  control_block_t *m_parent = {};      ///< Parent task (roots don't have one).
  std::int32_t m_steal = 0;            ///< Number of steals.

#ifndef NDEBUG
  std::int64_t m_debug_count = 0; ///< Number of forks/calls (debug).
#endif
};

static_assert(std::is_standard_layout_v<control_block_t>);

// ----------------------------------------------- //

/**
 * @brief A base class for all promise types that includes the type of the coroutine's return value and return address.
 *
 * If the coroutine is a root task then its return address is wrapped in a ``root_block_t``.
 *
 * @tparam R Type of the coroutine's return address.
 * @tparam T Type of the coroutine's return value.
 */
template <typename R, typename T>
struct promise_base;

template <>
struct promise_base<void, void> {
  //
  control_block_t m_control_block;

  static constexpr void return_void() noexcept {}
};

template <>
struct promise_base<root_block_t<void>, void> {
  //
  control_block_t m_control_block;

  root_block_t<void> *m_root_block;

  static constexpr void return_void() noexcept {}
};

template <typename R, typename T>
struct promise_base {
  //
  control_block_t m_control_block;

  template <typename U>
    requires std::constructible_from<T, U> && (std::is_void_v<R> || std::is_assignable_v<std::add_lvalue_reference_t<Ret>, U>)
  void return_value([[maybe_unused]] U &&expr) noexcept(std::is_void_v<Ret> || std::is_nothrow_assignable_v<std::add_lvalue_reference_t<Ret>, U>) {
    if constexpr (!std::is_void_v<Ret>) {
      this->get_return_address_obj() = std::forward<U>(expr);
    }
  };
};

} // namespace detail

// ----------------------------------------------- //

/**
 * @brief A handle to a task with a resume() member function.
 */
using task_handle = typename detail::control_block_t::handle_t;

// clang-format off

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of tasks.
 * The stack of ``lf::async_stack``s is expected to never be empty, it should always
 * be able to return an empty ``lf::async_stack``.
 *
 * \rst
 *
 * Specifically this requires:
 *
 * .. code::
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
concept thread_context =  requires(Context ctx, async_stack* stack, task_handle handle) {
  { Context::context() } -> std::same_as<Context &>; 

  { ctx.max_threads() } -> std::same_as<std::size_t>; 

  { ctx.stack_pop() }-> std::convertible_to<async_stack*>;                                                               
  { ctx.stack_push(stack) };                                                        

  { ctx.task_pop() } -> std::convertible_to<std::optional<task_handle>>; 
  { ctx.task_push(handle) };                                             
};

// clang-format on

// -------------- Define forward decls -------------- //

class detail::control_block_t::handle_t : private stdx::coroutine_handle<control_block_t> {
public:
  handle_t() = default; ///< To make us a trivial type.

  void resume() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(*this);

    stdx::coroutine_handle<control_block_t>::promise().m_steal += 1;
    stdx::coroutine_handle<control_block_t>::resume();
  }

private:
  template <typename R, typename T, thread_context Context, tag Tag>
  friend struct promise_type;

  explicit handle_t(stdx::coroutine_handle<control_block_t> handle) : stdx::coroutine_handle<control_block_t>{handle} {}
};

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
