module;
#include <version>

#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
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

export template <returnable T, worker_context Context>
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

  defer _ = [frame] noexcept -> void {
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
   * - OR had the task submitted to them.
   * - OR won the task at a join.
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

// =============== Fork/Call =============== //

template <category Cat, worker_context Context>
struct awaitable : std::suspend_always {

  using enum category;

  frame_type<Context> *child;

  // TODO: optional cancellation token

  template <typename T>
  constexpr auto
  await_suspend(this auto self, coro<promise_type<T, Context>> parent) noexcept(Cat == call) -> coro<> {

    if (!self.child) [[unlikely]] {
      // Noop if an exception was thrown
      return parent;
    }

    // TODO: handle cancellation

    // Propagate parent->child relationships
    self.child->parent = &parent.promise().frame;
    self.child->cancel = parent.promise().frame.cancel;
    self.child->stack_ckpt = not_null(thread_context<Context>)->alloc().checkpoint();
    self.child->kind = Cat;

    if constexpr (Cat == fork) {
      // It is critical to pass self by-value here, after the call to push()
      // the object `*this` may be destroyed, if passing by ref it would be
      // use-after-free to then access self in the following line to fetch the
      // handle.
      LF_TRY {
        not_null(thread_context<Context>)->push(frame_handle<Context>{key, &parent.promise().frame});
      } LF_CATCH_ALL {
        self.child->handle().destroy();
        // TODO: stash in parent frame
        LF_RETHROW;
      }
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

  template <category Cat, typename R, typename Fn, typename... Args>
  [[nodiscard]]
  static constexpr auto transform(pkg<R, Fn, Args...> &&pkg) noexcept -> frame_type<Context> * {
    LF_TRY {
      task child = std::move(pkg.args).apply(std::move(pkg.fn));

      LF_ASSUME(child.promise);

      if constexpr (!std::is_void_v<R>) {
        child.promise->return_address = pkg.return_address;
      }

      return &child.promise->frame;
    } LF_CATCH_ALL {
      // TODO: stash exception
      return nullptr;
    }
  }

  template <typename R, typename Fn, typename... Args>
  constexpr static auto
  await_transform(call_pkg<R, Fn, Args...> &&pkg) noexcept -> awaitable<category::call, Context> {
    return {.child = transform<category::call>(std::move(pkg))};
  }

  template <typename R, typename Fn, typename... Args>
  constexpr static auto
  await_transform(fork_pkg<R, Fn, Args...> &&pkg) noexcept -> awaitable<category::fork, Context> {
    return {.child = transform<category::fork>(std::move(pkg))};
  }

  constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  constexpr static auto final_suspend() noexcept -> final_awaitable { return {}; }

  constexpr static void unhandled_exception() noexcept {
    // TODO: stash exception in parent
    std::terminate();
  }
};

// =============== Promise (void) =============== //

template <worker_context Context>
struct promise_type<void, Context> : mixin_frame<Context> {

  frame_type<Context> frame;

  constexpr auto get_return_object() noexcept -> task<void, Context> { return {.promise = this}; }

  constexpr static void return_void() noexcept {}
};

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

} // namespace lf

// =============== std specialization =============== //

template <typename R, typename... Policy, typename... Args>
struct std::coroutine_traits<lf::task<R, Policy...>, Args...> {
  using promise_type = ::lf::promise_type<R, Policy...>;
};
