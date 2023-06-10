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
 * @brief Provides the pormise_type's common donominator.
 */

namespace lf {

// Stateless callables do not need to be captured, everything else does.

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_trivial_v<T> && std::is_empty_v<T> && std::is_class_v<T>;

/**
 * @brief The result type for ``lf::fn()``.
 *
 * Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct async_fn {};

/**
 * @brief Builds an async function from a stateless invocable that returns an ``lf::task``.
 */
template <stateless F>
constexpr auto fn(F) -> async_fn<F> { return {}; }

namespace detail {

// -------------- Tag types and constants -------------- //

template <typename T>
struct is_async_fn_impl : std::false_type {};

template <stateless Fn>
struct is_async_fn_impl<async_fn<Fn>> : std::true_type {};

template <typename T>
concept is_async_fn = is_async_fn_impl<T>::value;

//  Tags

struct root_t {};

struct call_t {};

struct call_from_root_t {};

struct fork_t {};

struct fork_from_root_t {};

template <typename T>
concept tag = std::same_as<T, root_t> || std::same_as<T, call_t> || std::same_as<T, call_from_root_t> || std::same_as<T, fork_t> || std::same_as<T, fork_from_root_t>;

/**
 * @brief An instance of this type is what is passed as the first argument to all coroutines.
 */
template <tag Tag, is_async_fn Wrap>
struct magic : Wrap {
  using tag = Tag;
};

static constexpr std::int32_t k_imax = std::numeric_limits<std::int32_t>::max();

// -------------- Control block definition -------------- //

class root_block_t : public exception_packet {
public:
  void acquire() noexcept {
    m_semaphore.acquire();
  }

  void release() noexcept {
    m_semaphore.release();
  }

private:
  std::binary_semaphore m_semaphore{0};
};

class control_block_t {
public:
  // Full declaration below, needs concept first
  class handle_t;

  explicit constexpr control_block_t(tag auto) noexcept : m_parent{nullptr} {
    LIBFORK_LOG("Constructing control block for child task");
  }
  explicit constexpr control_block_t(root_t) noexcept : m_root{nullptr} {
    LIBFORK_LOG("Constructing control block for root task");
  }

  constexpr void set(root_block_t &root) noexcept {
    m_root = &root; // NOLINT
  }
  constexpr void set(stdexp::coroutine_handle<control_block_t> parent) noexcept {
    m_parent = parent; // NOLINT
  }

  [[nodiscard]] constexpr auto parent() const noexcept -> stdexp::coroutine_handle<control_block_t> {
    LIBFORK_ASSERT(m_parent);
    return m_parent; // NOLINT
  }

  [[nodiscard]] constexpr auto root() const noexcept -> root_block_t & {
    LIBFORK_ASSERT(m_root);
    return *m_root; // NOLINT
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
  union {
    stdexp::coroutine_handle<control_block_t> m_parent; ///< Parent task.
    root_block_t *m_root;                               ///< or root block.
  };
  std::int32_t m_steal = 0;            ///< Number of steals.
  std::atomic_int32_t m_join = k_imax; ///< Number of children joined (obfuscated).
};

// -------------- promise_base -------------- //

// General case with return type.
template <typename T>
struct promise_base {

  template <typename U>
    requires std::assignable_from<T &, U>
  void return_value(U &&expr) noexcept(std::is_nothrow_assignable_v<T &, U>) {
    LIBFORK_LOG("Returning a value");
    LIBFORK_ASSERT(return_address != nullptr);
    *return_address = std::forward<U>(expr);
  }

  void set_return_address(T &ptr) noexcept {
    LIBFORK_LOG("Set return address");
    LIBFORK_ASSERT(return_address == nullptr);
    return_address = std::addressof(ptr);
  }

  control_block_t control_block; ///< Control block for this task.
  T *return_address = nullptr;   ///< Effectivly private but cannot mix member access for pointer interconvertibility.
};

// Special case for void return type.
template <>
struct promise_base<void> {
  static constexpr void return_void() noexcept {}

  control_block_t control_block; ///< Control block for this task.
};

} // namespace detail

/**
 * @brief A trivial handle to a task with a resume() member function.
 */
using task_handle = detail::control_block_t::handle_t;

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

class detail::control_block_t::handle_t : private stdexp::coroutine_handle<control_block_t> {
public:
  handle_t() = default; ///< To make us a trivial type.

  void resume() noexcept {
    LIBFORK_LOG("Call to resume on stolen task");
    LIBFORK_ASSERT(*this);

    stdexp::coroutine_handle<control_block_t>::promise().m_steal += 1;
    stdexp::coroutine_handle<control_block_t>::resume();
  }

private:
  template <typename T, thread_context Context, tag Tag>
  friend struct promise_type;

  explicit handle_t(stdexp::coroutine_handle<control_block_t> handle) : stdexp::coroutine_handle<control_block_t>{handle} {}
};

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
