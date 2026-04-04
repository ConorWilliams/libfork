module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:promise;

import std;

import :concepts;
import :frame;
import :utility;

import :thread_locals;
import :ops;

import :handles;

// TODO: vet constexpr usage in the library

namespace lf {

template <typename T = void>
using coro = std::coroutine_handle<T>;

template <worker_context Context>
using frame_t = frame_type<checkpoint_t<stack_t<Context>>>;

// =============== Forward-decl =============== //

// TODO: public private split here and elsewhere
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
class task {
 public:
  using value_type = T;
  using context_type = Context;

  constexpr task(key_t, promise_type<T, Context> *promise) noexcept : m_promise(promise) {}

 private:
  friend constexpr auto get(key_t, task t) noexcept -> promise_type<T, Context> * { return t.m_promise; }

  promise_type<T, Context> *m_promise;
};

// =============== Final =============== //

/**
 * @brief Part that deals only with parent.
 */
template <worker_context Context>
[[nodiscard]]
LF_NO_INLINE constexpr auto
final_suspend_continue(Context *context, frame_t<Context> *parent) noexcept -> coro<> {

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
  bool const owner = parent->stack_ckpt == context->stack().checkpoint();

  // TODO: we could reduce branching if we unconditionally release and also
  // drop pre-release function altogether... Need to benchmark with code that
  // triggers a lot of stealing.

  // As soon as we do the fetch_sub (if we loose) someone may acquire
  // the stack so we must prepare it for release now.
  auto release_key = context->stack().prepare_release();

  // TODO: we could add an `if (owner)` around acquire below, then we could
  // define that acquire is always called with null or not-self.

  // Register with parent we have completed this child task.
  if (parent->atomic_joins().fetch_sub(1, std::memory_order_release) == 1) {
    // Parent has reached join and we are the last child task to complete. We
    // are the exclusive owner of the parent and therefore, we must continue
    // parent. As we won the race, acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    if (!owner) {
      // In case of scenario (2) we must acquire the parent's stack.
      context->stack().acquire(std::as_const(parent->stack_ckpt));
    }

    // Must reset parent's control block before resuming parent.
    parent->reset_counters();

    return parent->handle();
  }

  // We did not win the join-race, we cannot dereference the parent pointer now
  // as the frame may now be freed by the winner. Parent has not reached join
  // or we are not the last child to complete. We are now out of jobs, we must
  // yield to the executor.

  if (owner) {
    // We were unable to resume the parent and we were its owner, as the
    // resuming thread will take ownership of the parent's we must give it up.
    context->stack().release(std::move(release_key));
  }

  // Else, case (2), our stack has no allocations on it, it may be used later.
  return std::noop_coroutine();
}

template <typename Checkpoint>
constexpr void finalize_root(frame_type<Checkpoint> *frame) noexcept {
  // Notify potential blockers
  not_null(frame->parent.block)->sem.release();
  // Release the refcount on the shared state
  release_ref(frame->parent.block);
  // Only now can we clean-up
  frame->handle().destroy();
}

template <worker_context Context>
[[nodiscard]]
constexpr auto final_suspend(frame_t<Context> *frame) noexcept -> coro<> {

  // Validate final state
  LF_ASSUME(frame->steals == 0);
  LF_ASSUME(frame->joins == k_u16_max);
  LF_ASSUME(frame->exception_bit == 0);

  // Handle root first/separately because:
  //  - It is unlikely
  //  - It allows simpler destroy logic
  if (frame->kind == category::root) [[unlikely]] {
    return finalize_root(frame), std::noop_coroutine();
  }

  // Safe to access union
  frame_t<Context> *parent = not_null(frame->parent.frame);

  // Read before destroy
  category const kind = frame->kind;

  // Can now destroy frame, must NOT touch from here on.
  frame->handle().destroy();

  switch (kind) {
    case category::root:
      LF_UNREACHABLE(); // Handled above
    case category::call:
      return parent->handle();
    case category::fork:

      Context *context = get_context<Context>();

      if (steal_handle<Context> last_pushed = context->pop()) {
        // No-one stole continuation, we are the exclusive owner of parent -> just keep ripping!
        LF_ASSUME(last_pushed == steal_handle<Context>{key(), parent});
        // This is not a join point so no state (i.e. counters) is guaranteed.
        return parent->handle();
      }
      // We split the function here as the remainder is the "slow-path", this
      // keeps the hot code as small as possible.
      // TODO: benchmark if this split/noinline regresses other in stealing context
      return final_suspend_continue(context, parent);
  }
}

struct final_awaitable : std::suspend_always {
  template <returnable T, worker_context Context>
  constexpr static auto await_suspend(coro<promise_type<T, Context>> handle) noexcept -> coro<> {
    return final_suspend<Context>(&handle.promise().frame);
  }
};

// =============== Fork/Call =============== //

/**
 * @brief Call inside a catch block, stash current exception in `frame`.
 */
template <typename Checkpoint>
constexpr void stash_current_exception(frame_type<Checkpoint> *frame) noexcept {
  // No synchronization is done via exception_bit, hence we can use relaxed atomics
  // and rely on the usual fork/join synchronization to ensure memory ordering.
  if (frame->atomic_except().exchange(1, std::memory_order_relaxed) == 0) {

    std::exception_ptr exception = std::current_exception();

    LF_ASSUME(exception); // Should have been called from inside a catch block

    // TODO: make sure exceptions are cancel-safe (I think now cancellation can leak)

    frame->except = new frame_type<Checkpoint>::except_type{
        .stashed = frame->parent,
        .exception = std::move(exception),
    };
  }
}

template <category Cat, worker_context Context>
struct awaitable : std::suspend_always {

  static_assert(Cat == category::call || Cat == category::fork, "Invalid category for awaitable");

  frame_t<Context> *child;

  /**
   * @brief In a separate function to allow it to be placed in cold block.
   */
  template <typename T>
  constexpr void stash_and_resume(this awaitable self, coro<promise_type<T, Context>> parent) noexcept {
    // Clean-up the child that will never be resumed.
    self.child->handle().destroy();
    stash_current_exception(&parent.promise().frame);
  }

  template <typename T>
  constexpr auto
  await_suspend(this awaitable self, coro<promise_type<T, Context>> parent) noexcept -> coro<> {

    // TODO: Add tests for exception/cancellation handling in fork/call.

    if (!self.child) [[unlikely]] {
      // Noop if an exception was thrown.
      return parent;
    }

    if (parent.promise().frame.is_cancelled()) [[unlikely]] {
      // Noop if canceled, must clean-up the child that will never be resumed.
      return self.child->handle().destroy(), parent;
    }

    // Propagate parent->child relationships
    self.child->parent.frame = &parent.promise().frame;
    self.child->cancel = parent.promise().frame.cancel;

    if constexpr (Cat == category::call) {
      // Should be the default
      LF_ASSUME(self.child->kind == category::call);
    } else {
      self.child->kind = Cat;
    }

    if constexpr (Cat == category::fork) {
      // It is critical to pass self by-value here, after the call to push()
      // the object `*this` may be destroyed, if passing by ref it would be
      // use-after-free to then access self in the following line to fetch the
      // handle.
      LF_TRY {
        get_context<Context>()->push(steal_handle<Context>{key(), &parent.promise().frame});
      } LF_CATCH_ALL {
        return self.stash_and_resume(parent), parent;
      }
    }

    return self.child->handle();
  }
};

// =============== Join =============== //

template <worker_context Context>
struct join_awaitable {

  using except_type = frame_t<Context>::except_type;

  frame_t<Context> *frame;

  constexpr auto take_stack_and_reset(this join_awaitable self) noexcept -> void {
    stack_t<Context> &stack = get_stack<Context>();
    LF_ASSUME(self.frame->stack_ckpt != stack.checkpoint());
    stack.acquire(std::as_const(self.frame->stack_ckpt));
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
      // For ruther explanation see await_suspend() below.
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

    // Lemma:
    //
    //    If a thread is at a join and steals have occurred then the
    //    thread can never own the stack of the current frame.
    //
    // This is because threads follow the work-first principle, so for the
    // owner to be running this task it would have to have re-stolen it from a
    // thief. Which implies it would have run the final suspend of the child
    // that had it's continuation stolen, where it would have had to release
    // the stack, because the parent was at not at the join.

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

  [[noreturn]]
  constexpr auto rethrow_exception(this join_awaitable self) -> void {
    // Local copy
    except_type except = std::move(*self.frame->except);

    // Clean-up exception state
    delete self.frame->except;
    self.frame->parent = except.stashed;
    self.frame->exception_bit = 0;

    std::rethrow_exception(std::move(except.exception));
  }

  constexpr void await_resume(this join_awaitable self) {
    // We should have been reset
    LF_ASSUME(self.frame->steals == 0);
    LF_ASSUME(self.frame->joins == k_u16_max);

    // Outside parallel regions so can touch non-atomically.
    if constexpr (LF_COMPILER_EXCEPTIONS) {
      if (self.frame->exception_bit) [[unlikely]] {
        self.rethrow_exception();
      }
    }
  }
};

// =============== Frame mixin =============== //

template <worker_context Context>
struct mixin_frame {

  // === For internal use === //

  using enum category;

  template <typename Self>
    requires (!std::is_const_v<Self>)
  [[nodiscard]]
  constexpr auto handle(this Self &self)
      LF_HOF(coro<Self>::from_promise(self))

  // === Called by the compiler === //

  // --- Allocation

  static auto operator new(std::size_t sz) noexcept(noexcept(get_stack<Context>().push(sz))) -> void * {
    void *ptr = get_stack<Context>().push(sz);
    LF_ASSUME(is_sufficiently_aligned<k_new_align>(ptr));
    return std::assume_aligned<k_new_align>(ptr);
  }

  static auto operator delete(void *p, std::size_t sz) noexcept -> void { get_stack<Context>().pop(p, sz); }

  // --- Await transformations

  template <category Cat, typename R, typename Fn, typename... Args>
  static constexpr auto await_transform_pkg(pkg<Cat, Context, R, Fn, Args...> &&pkg) noexcept(
      async_nothrow_invocable<Fn, Context, Args...>) -> awaitable<Cat, Context> {

    // Required for noexcept specifier to be correct
    static_assert(std::is_reference_v<Fn> && (... && std::is_reference_v<Args>));

    using U = async_result_t<Fn, Context, Args...>;

    // clang-format off

    promise_type<U, Context> *child_promise = get(key(), std::move(pkg.args).apply(
      [&](auto &&...args) LF_HOF(ctx_invoke_t<Context>{}(fwd_fn<Fn>(pkg.fn), LF_FWD(args)...))
    ));

    // clang-format on

    LF_ASSUME(child_promise);

    // void can signal drop return.
    static_assert(std::same_as<R, U> || std::is_void_v<R>);

    // TODO: tests for null path

    if constexpr (!std::is_void_v<R>) {
      child_promise->return_address = pkg.return_address;
    } else if constexpr (!std::is_void_v<U>) {
      // Set child's return address to null to inhibit the return
      // TODO: add test for this
      child_promise->return_address = nullptr;
    }

    return {.child = &child_promise->frame};
  }

  template <category Cat, typename R, typename Fn, typename... Args>
  constexpr auto await_transform(this auto &self, pkg<Cat, Context, R, Fn, Args...> &&pkg) noexcept
      -> awaitable<Cat, Context> {
    LF_TRY {
      return self.await_transform_pkg(std::move(pkg));
    } LF_CATCH_ALL {
      stash_current_exception(&self.frame);
    }
    return {.child = nullptr};
  }

  constexpr auto await_transform(this auto &self, join_type) noexcept -> join_awaitable<Context> {
    return {.frame = &self.frame};
  }

  constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  constexpr static auto final_suspend() noexcept -> final_awaitable { return {}; }

  constexpr void unhandled_exception(this auto &self) noexcept { stash_current_exception(&self.frame); }
};

// =============== Promise (void) =============== //

template <worker_context Context>
struct promise_type<void, Context> : mixin_frame<Context> {

  // Putting init here allows:
  //  1. Frame not no need to know about the checkpoint type
  //  2. Compiler merge double read of thread local here and in allocator
  frame_t<Context> frame{get_stack<Context>().checkpoint()};

  constexpr auto get_return_object() noexcept -> task<void, Context> { return {key(), this}; }

  constexpr static void return_void() noexcept {}
};

// =============== Promise (non-void) =============== //

template <returnable T, worker_context Context>
struct promise_type : mixin_frame<Context> {

  // Putting init here allows:
  //  1. Frame not no need to know about the checkpoint type
  //  2. Compiler merge double read of thread local here and in allocator
  frame_t<Context> frame{get_stack<Context>().checkpoint()};
  T *return_address;

  constexpr auto get_return_object() noexcept -> task<T, Context> { return {key(), this}; }

  template <typename U = T>
    requires std::assignable_from<T &, U &&>
  constexpr void return_value(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>) {
    if (return_address) {
      *return_address = LF_FWD(value);
    }
  }
};

} // namespace lf

// =============== std specialization =============== //

template <typename R, lf::worker_context Context, typename... Args>
struct std::coroutine_traits<lf::task<R, Context>, Args...> {
  using promise_type = ::lf::promise_type<R, Context>;
};

template <typename R, typename Self, lf::worker_context Context, typename... Args>
struct std::coroutine_traits<lf::task<R, Context>, Self, Args...> {
  using promise_type = ::lf::promise_type<R, Context>;
};
