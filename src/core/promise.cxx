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

  Context *context = not_null(thread_context<Context>);

  frame_type<Context> *parent = not_null(frame->parent);

  if (frame_handle last_pushed = context->pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, so we
    // just keep ripping!
    LF_ASSUME(last_pushed.m_ptr == parent);
    // This is not a join point so no state (i.e. counters) is gaurenteed.
    return parent->handle();
  }

  // An owner is a worker who:
  //
  // - Created the task.
  // - OR had the task submitted to them.
  // - OR won the task at a join.
  //
  // An owner of a task owns the stack the task is on.
  //
  // As the worker who completed the child task this thread owns the stack the child task was on.
  //
  // Either:
  //
  // 1. The parent is on the same stack as the child.
  // 2. OR the parent is on a different stack to the child.
  //
  // Case (1) implies: we owned the parent; forked the child task; then the parent was then stolen.
  // Case (2) implies: we stole the parent task; then forked the child; then the parent was stolen.
  //
  // Case (2) implies that our stack is empty.

  // As soon as we do the `fetch_sub` below the parent task is no longer safe
  // to access as it may be resumed and then destroyed by another thread. Hence
  // we must make copies on-the-stack of any data we may need if we lose the
  // join race.
  auto const checkpoint = parent->stack_ckpt;

  // Register with parent we have completed this child task.
  if (parent->atomic_joins().fetch_sub(1, std::memory_order_release) == 1) {
    // Parent has reached join and we are the last child task to complete. We
    // are the exclusive owner of the parent and therefore, we must continue
    // parent. As we won the race, acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    // In case of scenario (2) we must acquire the parent's stack.
    context->alloc().acquire(checkpoint);

    // Must reset parent's control block before resuming parent.
    parent->reset_counters();

    return parent->handle();
  }

  // We did not win the join-race, we cannot dereference the parent pointer now
  // as the frame may now be freed by the winner. Parent has not reached join
  // or we are not the last child to complete. We are now out of jobs, we must
  // yield to the executor.

  if (checkpoint == context->alloc().checkpoint()) {
    // We were unable to resume the parent and we were its owner, as the
    // resuming thread will take ownership of the parent's we must give it up.
    context->alloc().release();
  }

  // Else, case (2), our stack has no allocations on it, it may be used later.
  // TODO: assert empty

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

// =============== Join =============== //

template <worker_context Context>
struct join_awaitable {

  frame_type<Context> *frame;

  constexpr auto take_stack_and_reset(this join_awaitable self) noexcept -> void {
    Context *context = not_null(thread_context<Context>);
    LF_ASSUME(self.frame->stack_ckpt != context->alloc().checkpoint());
    context->alloc().acquire(std::as_const(self.frame->stack_ckpt));
    self.frame->reset_counters();
  }

  constexpr auto await_ready(this join_awaitable self) noexcept -> bool {
    if (not_null(self.frame)->steals == 0) [[likely]] {
      // If no steals then we are the only owner of the parent and we are ready
      // to join. Therefore, no need to reset the control block.
      return true;
    }

    // TODO: benchmark if including the below check (returning false here) in
    // multithreaded case helps performance enough to justify the extra
    // instructions along the fast path

    // Currently:               joins() = k_u16_max - num_joined
    // Hence:       k_u16_max - joins() = num_joined

    // Could use (relaxed here) + (fence(acquire) in truthy branch) but, it's
    // better if we see all the decrements to joins() and avoid suspending the
    // coroutine if possible. Cannot fetch_sub() here and write to frame as
    // coroutine must be suspended first.

    std::uint32_t steals = self.frame->steals;
    std::uint32_t joined = k_u16_max - self.frame->atomic_joins().load(std::memory_order_acquire);

    if (steals == joined) {
      // We must reset the control block and take the stack. We should never
      // own the stack at this point because we must have stolen the stack.
      return self.take_stack_and_reset(), true;
    }

    return false;
  }

  constexpr auto await_suspend(this join_awaitable self, std::coroutine_handle<> task) noexcept -> coro<> {
    // Currently   self.joins  = k_u16_max  - num_joined
    // We set           joins  = self->joins - (k_u16_max - num_steals)
    //                         = num_steals - num_joined

    // Hence               joined = k_u16_max - num_joined
    //         k_u16_max - joined = num_joined

    LF_ASSUME(self.frame);

    std::uint32_t steals = self.frame->steals;
    std::uint32_t offset = k_u16_max - steals;
    std::uint32_t joined = self.frame->atomic_joins().fetch_sub(offset, std::memory_order_release);

    if (steals == k_u16_max - joined) {
      // We set joins after all children had completed therefore we can resume the task.
      // Need to acquire to ensure we see all writes by other threads to the result.
      std::atomic_thread_fence(std::memory_order_acquire);

      // We must reset the control block and take the stack. We should never
      // own the stack at this point because we must have stolen the stack.
      return self.take_stack_and_reset(), task;
    }

    // Someone else is responsible for running this task.
    // We cannot touch *this or dereference self as someone may have resumed already!
    // We cannot currently own this stack (checking would violate above).

    // If no explicit scheduling then we must have an empty WSQ as we stole this task.

    // If explicit scheduling then we may have tasks on our WSQ if we performed a self-steal
    // in a switch awaitable. In this case we can/must do another self-steal.

    // return try_self_stealing();
    return std::noop_coroutine();
  }

  constexpr void await_resume(this join_awaitable self) {
    // We should have been reset
    LF_ASSUME(self.frame->steals == 0);
    LF_ASSUME(self.frame->joins == k_u16_max);

    if constexpr (LF_COMPILER_EXCEPTIONS) {
      if (self.frame->exception_bit) {
        LF_THROW(std::runtime_error{"Child task threw an exception"});
      }
    }
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

  constexpr auto
  await_transform(this auto &self, join_type tag [[maybe_unused]]) noexcept -> join_awaitable<Context> {
    return {.frame = &self.frame};
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
