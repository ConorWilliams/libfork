#ifndef C854CDE9_1125_46E1_9E2A_0B0006BFC135
#define C854CDE9_1125_46E1_9E2A_0B0006BFC135

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic> // for atomic_thread_fence, mem...
#include <bit>    // for bit_cast
#include <concepts>
#include <coroutine>   // for coroutine_handle, noop_c...
#include <cstddef>     // for size_t
#include <type_traits> // for false_type, remove_cvref_t
#include <utility>     // for forward

#include "libfork/core/co_alloc.hpp"        // for co_allocable, co_new_t
#include "libfork/core/control_flow.hpp"    // for join_type
#include "libfork/core/exception.hpp"       // for stash_exception_in_return
#include "libfork/core/ext/context.hpp"     // for full_context
#include "libfork/core/ext/handles.hpp"     // for submit_t, task_handle
#include "libfork/core/ext/list.hpp"        // for intrusive_list
#include "libfork/core/ext/tls.hpp"         // for stack, context
#include "libfork/core/first_arg.hpp"       // for async_function_object
#include "libfork/core/impl/awaitables.hpp" // for alloc_awaitable, call_aw...
#include "libfork/core/impl/combinate.hpp"  // for quasi_awaitable
#include "libfork/core/impl/frame.hpp"      // for frame
#include "libfork/core/impl/return.hpp"     // for return_result
#include "libfork/core/impl/stack.hpp"      // for stack
#include "libfork/core/impl/utility.hpp"    // for byte_cast, k_u16_max
#include "libfork/core/invocable.hpp"       // for return_address_for, igno...
#include "libfork/core/just.hpp"            // for just_awaitable
#include "libfork/core/macro.hpp"           // for LF_LOG, LF_ASSERT, LF_FO...
#include "libfork/core/scheduler.hpp"       // for context_switcher
#include "libfork/core/tag.hpp"             // for tag
#include "libfork/core/task.hpp"            // for returnable, task

/**
 * @file promise.hpp
 *
 * @brief The promise type for all tasks/coroutines.
 */

namespace lf::impl {

namespace detail {

inline auto final_await_suspend(frame *parent) noexcept -> std::coroutine_handle<> {

  full_context *context = tls::context();

  if (task_handle parent_task = context->pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
    LF_LOG("Parent not stolen, keeps ripping");
    LF_ASSERT(byte_cast(parent_task) == byte_cast(parent));
    // This must be the same thread that created the parent so it already owns the stack.
    // No steals have occurred so we do not need to call reset().;
    return parent->self();
  }

  /**
   * An owner is a worker who:
   *
   * - Created the task.
   * - Had the task submitted to them.
   * - Won the task at a join.
   *
   * An owner of a task owns the stack the task is on.
   *
   * As the worker who completed the child task this thread owns the stack the child task was on.
   *
   * Either:
   *
   * 1. The parent is on the same stack as the child.
   * 2. The parent is on a different stack to the child.
   *
   * Case (1) implies: we owned the parent; forked the child task; then the parent was then stolen.
   * Case (2) implies: we stole the parent task; then forked the child; then the parent was stolen.
   *
   * In case (2) the workers stack has no allocations on it.
   */

  LF_LOG("Task's parent was stolen");

  stack *tls_stack = tls::stack();

  stack::stacklet *p_stacklet = parent->stacklet(); //
  stack::stacklet *c_stacklet = tls_stack->top();   // Need to call while we own tls_stack.

  // Register with parent we have completed this child task, this may release ownership of our stack.
  if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
    // Acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    // Parent has reached join and we are the last child task to complete.
    // We are the exclusive owner of the parent therefore, we must continue parent.

    LF_LOG("Task is last child to join, resumes parent");

    if (p_stacklet != c_stacklet) {
      // Case (2), the tls_stack has no allocations on it.

      LF_ASSERT(tls_stack->empty());

      // TODO: stack.splice()? Here the old stack is empty and thrown away, if it is larger
      // then we could splice it onto the parents one? Or we could attempt to cache the old one.
      *tls_stack = stack{p_stacklet};
    }

    // Must reset parents control block before resuming parent.
    parent->reset();

    return parent->self();
  }

  // We did not win the join-race, we cannot deference the parent pointer now as
  // the frame may now be freed by the winner.

  // Parent has not reached join or we are not the last child to complete.
  // We are now out of jobs, must yield to executor.

  LF_LOG("Task is not last to join");

  if (p_stacklet == c_stacklet) {
    // We are unable to resume the parent and where its owner, as the resuming
    // thread will take ownership of the parent's we must give it up.
    LF_LOG("Thread releases control of parent's stack");

    // If this throw an exception then the worker must die as it does not have a stack.
    // Hence, program termination is appropriate.
    ignore_t{} = tls_stack->release();

  } else {
    // Case (2) the tls_stack has no allocations on it, it may be used later.
  }

  return std::noop_coroutine();
}

} // namespace detail

/**
 * @brief Type independent bits
 */
struct promise_base : frame {

  /**
   * @brief Inherit constructor.
   */
  using frame::frame;

  /**
   * @brief Allocate the coroutine on a new stack.
   */
  LF_FORCEINLINE static auto operator new(std::size_t size) -> void * { return tls::stack()->allocate(size); }

  /**
   * @brief Deallocate the coroutine from current `stack`s stack.
   */
  LF_FORCEINLINE static void operator delete(void *ptr) noexcept { tls::stack()->deallocate(ptr); }

  /**
   * @brief Assert destroyed by the correct thread.
   */
  ~promise_base() noexcept { LF_ASSERT(tls::stack()->top() == stacklet()); }

  /**
   * @brief Start suspended (lazy).
   */
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  // -------------------------------------------------------------- //

  /**
   * @brief Make an awaitable that allocates on this workers stack.
   */
  template <co_allocable T>
  auto await_transform(co_new_t<T> await) noexcept {
    return alloc_awaitable<T>{{}, await, this};
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Transform a context_switch awaitable into a real awaitable.
   */
  template <context_switcher A>
  auto await_transform(A &&await) -> context_switch_awaitable<std::remove_cvref_t<A>> {

    auto *submit = std::bit_cast<impl::submit_t *>(static_cast<frame *>(this));

    using node = typename intrusive_list<impl::submit_t *>::node;

    return {std::forward<A>(await), node{submit}};
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Get a join awaitable.
   */
  auto await_transform(join_type /*unused*/) noexcept -> join_awaitable { return {this}; }

  // -------------------------------------------------------------- //

  /**
   * @brief Transform a call packet into a call awaitable.
   */
  template <returnable R2, return_address_for<R2> I2, tag Tg>
    requires (Tg == tag::call || Tg == tag::fork)
  auto await_transform(quasi_awaitable<R2, I2, Tg> &&awaitable) noexcept {

    awaitable.prom->set_parent(this);

    if constexpr (Tg == tag::call) {
      return call_awaitable{{}, awaitable.prom};
    }

    if constexpr (Tg == tag::fork) {
      return fork_awaitable{{}, awaitable.prom, this};
    }
  }

  // -------------------------------------------------------------- //

  /**
   * @brief Pass through a just awaitable.
   */
  template <returnable R2>
  auto await_transform(just_awaitable<R2> &&awaitable) noexcept -> just_awaitable<R2> && {
    awaitable.frame()->set_parent(this);
    return std::move(awaitable);
  }

  /**
   * @brief Pass through a just awaitable.
   */
  template <returnable T>
  auto await_transform(just_wrapped<T> &&awaitable) noexcept -> just_wrapped<T> && {
    return std::move(awaitable);
  }
};

/**
 * @brief The promise type for all tasks/coroutines.
 *
 * @tparam R The type of the return address.
 * @tparam T The value type of the coroutine (what it promises to return).
 * @tparam Context The type of the context this coroutine is running on.
 * @tparam Tag The dispatch tag of the coroutine.
 */
template <returnable R, return_address_for<R> I, tag Tag>
struct promise : promise_base, return_result<R, I> {

  static_assert(Tag != tag::root || stash_exception_in_return<I>);

  /**
   * @brief Construct a new promise object, delegate to main constructor.
   */
  template <typename This, first_arg Arg, typename... Args>
  promise(This const & /*unused*/, Arg &arg, Args const &.../*unused*/) noexcept : promise(arg) {}

  /**
   * @brief Construct a new promise object.
   *
   * Stores a handle to the promise in the `frame` and loads the tls stack
   * and stores a pointer to the top fibril. Also sets the first argument's frame pointer.
   */
  template <first_arg Arg, typename... Args>
  explicit promise(Arg &arg, Args const &.../*unused*/) noexcept
      : promise_base{std::coroutine_handle<promise>::from_promise(*this), tls::stack()->top()} {
    unsafe_set_frame(arg, this);
  }

  /**
   * @brief Returned task stores a copy of the `this` pointer.
   */
  auto get_return_object() noexcept -> task<R> { return {{}, static_cast<void *>(this)}; }

  /**
   * @brief Try to resume the parent.
   */
  auto final_suspend() const noexcept {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the stack_stack this child is on

    LF_ASSERT(this->load_steals() == 0);                                           // Fork without join.
    LF_ASSERT_NO_ASSUME(this->load_joins(std::memory_order_acquire) == k_u16_max); // Invalid state.
    LF_ASSERT(!this->has_exception());                                             // Must have rethrown.

    return final_awaitable{};
  }

  /**
   * @brief Cache in parent's stacklet.
   */
  void unhandled_exception() noexcept {
    if constexpr (stash_exception_in_return<I>) {
      stash_exception(*(this->get_return()));
    } else {
      this->parent()->capture_exception();
    }
  }

 private:
  struct final_awaitable : std::suspend_always {
    static auto await_suspend(std::coroutine_handle<promise> child) noexcept -> std::coroutine_handle<> {

      if constexpr (Tag == tag::root) {

        LF_LOG("Root task at final suspend, releases semaphore and yields");

        child.promise().semaphore()->release();
        child.destroy();

        // A root task is always the first on a stack, now it has been completed the stack is empty.
        LF_ASSERT(tls::stack()->empty());

        return std::noop_coroutine();
      }

      LF_LOG("Task reaches final suspend, destroying child");

      frame *parent = child.promise().parent();
      child.destroy();

      if constexpr (Tag == tag::call) {
        LF_LOG("Inline task resumes parent");
        // Inline task's parent cannot have been stolen as its continuation was not
        // pushed to a queue hence, no need to reset control block. We do not
        // attempt to take the stack because stack-eats only occur at a sync point.
        return parent->self();
      }

      return detail::final_await_suspend(parent);
    }
  };
};

// -------------------------------------------------- //

/**
 * @brief A basic type list.
 */
template <typename...>
struct list {};

namespace detail {

/**
 * @brief A dependent value to emulate `static_assert(false)`.
 */
template <typename...>
inline constexpr bool always_false = false;

/**
 * @brief All non-reference destinations are safe for most types.
 */
template <typename From, typename To>
struct safe_fork_t : std::false_type {
  static_assert(always_false<From, To>, "Unsafe fork detected!");
};

/**
 * @brief Pass by value is (in general) safe.
 *
 * This may not hold if the type is a reference wrapper of some kind.
 */
template <typename From, typename To>
  requires (!std::is_reference_v<To>)
struct safe_fork_t<From, To> : std::true_type {};

/**
 * @brief l-value references are safe.
 */
template <typename From, typename To>
  requires std::convertible_to<From &, To &>
struct safe_fork_t<From &, To &> : std::true_type {};

/**
 * @brief Const promotion of l-value references is safe.
 */
template <typename From, typename To>
  requires std::convertible_to<From &, To &>
struct safe_fork_t<From &, To const &> : std::true_type {};

/**
 * @brief Triggers a static assert if a conversion may dangle.
 */
template <tag, typename, typename>
struct safe_fork : std::true_type {};

// General case.
template <typename From, typename... A, typename To, typename... B>
struct safe_fork<tag::fork, list<From, A...>, list<To, B...>> : safe_fork<tag::fork, list<A...>, list<B...>> {
  static_assert(safe_fork_t<From, To>::value);
};

/**
 * @brief Special case for defaulted arguments, can only check if they are r-values references.
 */
template <typename Head, typename... Tail>
struct safe_fork<tag::fork, list<>, list<Head, Tail...>> : safe_fork<tag::fork, list<>, list<Tail...>> {
  static_assert(!std::is_rvalue_reference_v<Head>, "Forked rvalue will dangle");
};

} // namespace detail

/**
 * @brief Triggers a static assert if a conversion may dangle.
 */
template <tag Tag, typename FromList, typename ToList>
inline constexpr bool safe_fork_v = detail::safe_fork<Tag, FromList, ToList>::value;

} // namespace lf::impl

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <lf::returnable R,
          lf::impl::return_address_for<R> I,
          lf::tag Tag,
          lf::async_function_object F,
          typename... CallArgs,
          typename... Args>
struct std::coroutine_traits<lf::task<R>, lf::impl::first_arg_t<I, Tag, F, CallArgs...>, Args...> {

  // May have less if defaulted parameters are used.
  static_assert(sizeof...(CallArgs) <= sizeof...(Args));

  // This will trigger an inner static assert if an unsafe reference is forked.
  static_assert(lf::impl::safe_fork_v<Tag, lf::impl::list<CallArgs...>, lf::impl::list<Args...>>);

  using promise_type = lf::impl::promise<R, I, Tag>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <lf::returnable R,
          typename This,
          lf::impl::return_address_for<R> I,
          lf::tag Tag,
          lf::async_function_object F,
          typename... CallArgs,
          typename... Args>
struct std::coroutine_traits<lf::task<R>, This, lf::impl::first_arg_t<I, Tag, F, CallArgs...>, Args...>
    : std::coroutine_traits<lf::task<R>, lf::impl::first_arg_t<I, Tag, F, CallArgs...>, Args..., This> {};

#endif

#endif /* C854CDE9_1125_46E1_9E2A_0B0006BFC135 */
