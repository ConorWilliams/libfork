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
#include <exception>
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

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

/**
 * @brief The result type for ``lf::fn()``.
 *
 * Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct async_fn : std::type_identity<Fn> {};

/**
 * @brief The result type for ``lf::mem_fn()``.
 *
 * Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct async_mem_fn : std::type_identity<Fn> {};

/**
 * @brief Builds an async function from a stateless invocable that returns an ``lf::task``.
 *
 * Use this to define a global function which is passed a copy of itself as its first parameter (e.g. a y-combinator).
 */
template <stateless F>
consteval auto fn([[maybe_unused]] F invocable_which_returns_a_task) -> async_fn<F> { return {}; }

/**
 * @brief Builds an async function from a stateless invocable that returns an ``lf::task``.
 *
 * Use this to define a member function which is passed the class as its first parameter.
 */
template <stateless F>
consteval auto mem_fn([[maybe_unused]] F invocable_which_returns_a_task) -> async_mem_fn<F> { return {}; }

namespace detail {

// -------------- Tag types and constants -------------- //

template <typename T>
struct is_async_fn_impl : std::false_type {};

template <stateless Fn>
struct is_async_fn_impl<async_fn<Fn>> : std::true_type {};

template <stateless Fn>
struct is_async_fn_impl<async_mem_fn<Fn>> : std::true_type {};

} // namespace detail

/**
 * @brief A concept to test if a type is an async function.
 */
template <typename T>
concept async_wrapper = detail::is_async_fn_impl<T>::value;

namespace detail {

//  Tags

enum class tag {
  root,        ///< This coro is a root task (allocated on heap). [pointer to a root block, constructs]
  call,        ///< Non root task (on a virtual stack) from a lf::call.  [pointer to result, assigns]
  call_return, ///< Non root task (on a virtual stack) from an lf::call that will return. [pointer to result block, constructs]
  fork,        ///< Non root task (on a virtual stack) from an lf::fork. [pointer to result, assigns]
};

/**
 * @brief An instance of this type is what is passed as the first argument to all coroutines.
 */
template <tag Tag, typename...>
class magic;

// Sepcialisation for global functions
template <tag Tag, stateless F>
class magic<Tag, async_fn<F>> : public async_fn<F> {
  static constexpr tag tag = Tag;
};

/**
 * @brief Specialisation for member functions.
 *
 * fn(int x, int const y)
 *
 *          call(x) -> int &
 *    call(move(x)) -> int
 *
 *          call(x) -> int const &
 *    call(move(x)) -> int const
 */
template <tag Tag, stateless F, typename This>
class magic<Tag, async_mem_fn<F>, This> {
public:
  explicit constexpr magic(This &self) : m_self{std::addressof(self)} {}

  static constexpr tag tag = Tag;

  // [[nodiscard]] constexpr auto operator->() const noexcept -> This * { return m_self; }

  [[nodiscard]] constexpr auto operator*() & noexcept -> std::remove_reference_t<This> & {
    return *m_self;
  }

  [[nodiscard]] constexpr auto operator*() && noexcept -> std::remove_reference_t<This> && {
    return std::move(*m_self);
  }

private:
  This m_self;
};

static constexpr std::int32_t k_imax = std::numeric_limits<std::int32_t>::max();

// -------------- Control block definition -------------- //

// /**
//  * @brief Store an (uninitialised) T and an exception_ptr.
//  */
// template <typename T>
// class deferred {
// public:
//   auto get() && -> std::conditional_t<std::is_void_v<T>, void, T> {
//     if (m_exception) {
//       std::rethrow_exception(m_exception);
//     }
//     if constexpr (!std::is_void_v<T>) {
//       return std::move(m_uninit.m_value);
//     } else {
//       return;
//     }
//   }

//   // clang-format off

//   ~deferred() requires(std::is_trivially_destructible_v<T> || std::is_void_v<T>) = default;

//   // clang-format on

//   ~deferred() {
//     if (!m_exception) {
//       std::destroy_at(std::addressof(m_uninit.m_value)); // NOLINT
//     }
//   }

//   struct empty {};

//   union uninitialised {
//     empty m_init = {};
//     T m_value;
//   };

//   std::exception_ptr m_exception;
//   [[no_unique_address]] std::conditional_t<std::is_void_v<T>, empty, uninitialised> m_uninit{};
// };

// -------------- Control block definition -------------- //

template <typename T>
struct root_block_t : exception_packet {
  std::binary_semaphore m_semaphore{0};
  std::optional<T> m_result{};
};

template <>
struct root_block_t<void> : exception_packet {
  std::binary_semaphore m_semaphore{0};
};

class promise_base {
public:
  // Full declaration below, needs concept first
  class handle_t;

  constexpr void set(void *ret) noexcept {
    m_return_address = ret;
  }
  constexpr void set(stdexp::coroutine_handle<promise_base> parent) noexcept {
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
  stdexp::coroutine_handle<promise_base> m_parent = {}; ///< Parent task (roots dont have one).
  void *m_return_address = nullptr;                     ///< root_block || T * || defered<T> *
  std::int32_t m_steal = 0;                             ///< Number of steals.
  std::atomic_int32_t m_join = k_imax;                  ///< Number of children joined (obfuscated).
};

// -------------- promise_base -------------- //

} // namespace detail

/**
 * @brief A trivial handle to a task with a resume() member function.
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
