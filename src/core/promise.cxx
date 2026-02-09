module;
#include <version>

#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:promise;

import std;

import :concepts;
import :frame;
import :utility;

import :context;
import :ops;

namespace lf {

template <typename T = void>
using coro = std::coroutine_handle<T>;

// =============== Forward-decl =============== //

template <returnable T, worker_context Context>
struct promise_type;

// =============== Task =============== //

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other
 * coroutines and specify `T` the async function's return type which is
 * required to be `void` or a `std::movable` type.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should ever touch an instance of this type,
 *    it is used for specifying the return type of an `async` function only.
 *
 * \endrst
 */
export template <returnable T, worker_context Context>
struct task : immovable {

  // TODO: public private split here and elsewhere

  using value_type = T;
  using context_type = Context;

  promise_type<T, Context> *promise;
};

// =============== Final =============== //

template <worker_context Context>
[[nodiscard]]
constexpr auto final_suspend(frame_type<Context> *frame) noexcept -> coro<> {

  defer _ = [frame]() noexcept -> void {
    frame->handle().destroy();
  };

  switch (not_null(frame)->kind) {
    case category::call:
      return not_null(frame->parent)->handle();
    case category::root:
      // TODO: root handling
      return std::noop_coroutine();
    case category::fork:
      break;
    default:
      LF_ASSUME(false);
  }
  auto *context = not_null(thread_context<Context>);
  auto *parent = not_null(frame->parent);

  if (frame_handle last_pushed = context->pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, so we
    // just keep ripping!
    LF_ASSUME(last_pushed.m_ptr == parent);
    // If no-one stole the parent then this child can also never have been
    // stolen. Hence, this must be the same thread that created the parent so
    // it already owns the stack. No steals have occurred so we do not need to
    // call reset().
    // TODO: assert about the stack
    return parent->handle();
  }

  LF_TERMINATE("oops");

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

  // LF_LOG("Task's parent was stolen");
  //
  // stack *tls_stack = tls::stack();
  //
  // stack::stacklet *p_stacklet = parent->stacklet(); //
  // stack::stacklet *c_stacklet = tls_stack->top();   // Need to call while we own tls_stack.
  //
  // // Register with parent we have completed this child task, this may release ownership of our stack.
  // if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
  //   // Acquire all writes before resuming.
  //   std::atomic_thread_fence(std::memory_order_acquire);
  //
  //   // Parent has reached join and we are the last child task to complete.
  //   // We are the exclusive owner of the parent therefore, we must continue parent.
  //
  //   LF_LOG("Task is last child to join, resumes parent");
  //
  //   if (p_stacklet != c_stacklet) {
  //     // Case (2), the tls_stack has no allocations on it.
  //
  //     LF_ASSERT(tls_stack->empty());
  //
  //     // TODO: stack.splice()? Here the old stack is empty and thrown away, if it is larger
  //     // then we could splice it onto the parents one? Or we could attempt to cache the old one.
  //     *tls_stack = stack{p_stacklet};
  //   }
  //
  //   // Must reset parents control block before resuming parent.
  //   parent->reset();
  //
  //   return parent->self();
  // }
  //
  // // We did not win the join-race, we cannot deference the parent pointer now as
  // // the frame may now be freed by the winner.
  //
  // // Parent has not reached join or we are not the last child to complete.
  // // We are now out of jobs, must yield to executor.
  //
  // LF_LOG("Task is not last to join");
  //
  // if (p_stacklet == c_stacklet) {
  //   // We are unable to resume the parent and where its owner, as the resuming
  //   // thread will take ownership of the parent's we must give it up.
  //   LF_LOG("Thread releases control of parent's stack");
  //
  //   // If this throw an exception then the worker must die as it does not have a stack.
  //   // Hence, program termination is appropriate.
  //   ignore_t{} = tls_stack->release();
  //
  // } else {
  //   // Case (2) the tls_stack has no allocations on it, it may be used later.
  // }

  return std::noop_coroutine();
}

struct final_awaitable : std::suspend_always {
  template <returnable T, worker_context Context>
  constexpr static auto await_suspend(coro<promise_type<T, Context>> handle) noexcept -> coro<> {
    return final_suspend<Context>(&handle.promise().frame);
  }
};

// =============== Call =============== //

template <worker_context Context>
struct call_awaitable : std::suspend_always {

  frame_type<Context> *child;

  template <returnable T>
  auto await_suspend(this auto self, coro<promise_type<T, Context>> parent) noexcept -> coro<> {
    // Connect child to parent
    not_null(self.child)->parent = &parent.promise().frame;

    return self.child->handle();
  }
};

// =============== Fork =============== //

template <worker_context Context>
struct fork_awaitable : std::suspend_always {

  frame_type<Context> *child;

  template <typename T>
  auto await_suspend(this auto self, coro<promise_type<T, Context>> parent) noexcept -> coro<> {

    // It is critical to pass self by-value here, after the call to push()
    // the object may be destroyed, if passing by ref it would be use
    // after-free to then access self

    // TODO: destroy on child if cannot launch i.e. scheduling failure

    not_null(self.child)->parent = &parent.promise().frame;

    LF_TRY {
      not_null(thread_context<Context>)->push(frame_handle<Context>{key, &parent.promise().frame});
    } LF_CATCH_ALL {
      self.child->handle().destroy();
      // TODO: stash in parent frame (should not throw)
      LF_RETHROW;
    }

    return self.child->handle();
  }
};

// =============== Frame mixin =============== //

template <worker_context Context>
struct mixin_frame {

  // === For internal use === //

  template <typename Self>
    requires (!std::is_const_v<Self>)
  [[nodiscard]]
  constexpr auto handle(this Self &self)
      LF_HOF(coro<Self>::from_promise(self))

  // === Called by the compiler === //

  // --- Allocation

  static auto operator new(std::size_t sz) -> void * {
    return not_null(thread_context<Context>)->alloc().push(sz);
  }

  static auto operator delete(void *p, std::size_t sz) noexcept -> void {
    not_null(thread_context<Context>)->alloc().pop(p, sz);
  }

  // --- Await transformations

  template <typename R, typename Fn, typename... Args>
  static constexpr auto await_transform(call_pkg<R, Fn, Args...> &&pkg) noexcept -> call_awaitable<Context> {

    task child = std::move(pkg.args).apply(std::move(pkg.fn));

    // ::call is the default value
    LF_ASSUME(child.promise->frame.kind == category::call);

    child.promise->frame.stack_ckpt = not_null(thread_context<Context>)->alloc().checkpoint();

    if constexpr (!std::is_void_v<R>) {
      child.promise->return_address = pkg.return_address;
    }

    return {.child = &child.promise->frame};
  }

  template <typename R, typename Fn, typename... Args>
  constexpr static auto await_transform(fork_pkg<R, Fn, Args...> &&pkg) noexcept -> fork_awaitable<Context> {

    task child = std::move(pkg.args).apply(std::move(pkg.fn));

    child.promise->frame.kind = category::fork;

    child.promise->frame.stack_ckpt = not_null(thread_context<Context>)->alloc().checkpoint();

    if constexpr (!std::is_void_v<R>) {
      child.promise->return_address = pkg.return_address;
    }

    return {.child = &child.promise->frame};
  }

  constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  constexpr static auto final_suspend() noexcept -> final_awaitable { return {}; }

  constexpr static void unhandled_exception() noexcept { std::terminate(); }
};

// static_assert(std::is_empty_v<mixin_frame>);

// =============== Promise (void) =============== //

template <worker_context Context>
struct promise_type<void, Context> : mixin_frame<Context> {

  frame_type<Context> frame;

  constexpr auto get_return_object() noexcept -> task<void, Context> { return {.promise = this}; }

  constexpr static void return_void() noexcept {}
};

// TODO: move this and other static_asserts to a test file

// struct dummy_alloc {
//   static auto operator new(std::size_t) -> void *;
//   static auto operator delete(void *, std::size_t) noexcept -> void;
// };

// static_assert(alignof(promise_type<void, dummy_alloc>) == alignof(frame_type));
//
// #ifdef __cpp_lib_is_pointer_interconvertible
// static_assert(std::is_pointer_interconvertible_with_class(&promise_type<void, dummy_alloc>::frame));
// #else
// static_assert(std::is_standard_layout_v<promise_type<void, dummy_alloc>>);
// #endif

// =============== Promise (non-void) =============== //

template <returnable T, worker_context Context>
struct promise_type : mixin_frame<Context> {

  frame_type<Context> frame;
  T *return_address;

  constexpr auto get_return_object() noexcept -> task<T, Context> { return {.promise = this}; }

  template <typename U = T>
    requires std::assignable_from<T &, U &&>
  constexpr void return_value(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>) {
    *return_address = LF_FWD(value);
  }
};

// static_assert(alignof(promise_type<int, dummy_alloc>) == alignof(frame_type));
//
// #ifdef __cpp_lib_is_pointer_interconvertible
// static_assert(std::is_pointer_interconvertible_with_class(&promise_type<int, dummy_alloc>::frame));
// #else
// static_assert(std::is_standard_layout_v<promise_type<int, dummy_alloc>>);
// #endif

} // namespace lf

// =============== std specialization =============== //

template <typename R, typename... Policy, typename... Args>
struct std::coroutine_traits<lf::task<R, Policy...>, Args...> {
  using promise_type = ::lf::promise_type<R, Policy...>;
};
