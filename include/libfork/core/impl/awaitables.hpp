#ifndef CF3E6AC4_246A_4131_BF7A_FE5CD641A19B
#define CF3E6AC4_246A_4131_BF7A_FE5CD641A19B

#include "libfork/core/ext/handles.hpp"
#include "libfork/core/ext/tls.hpp"

#include "libfork/core/impl/frame.hpp"
#include "libfork/core/impl/utility.hpp"

namespace lf::impl {

// -------------------------------------------------------- //

struct switch_awaitable : std::suspend_always {

  auto await_ready() const noexcept { return tls::context() == dest; }

  void await_suspend(std::coroutine_handle<>) noexcept { dest->submit(&self); }

  intrusive_list<submit_handle>::node self;
  context *dest;
};

// -------------------------------------------------------- //

struct fork_awaitable : std::suspend_always {

  auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Forking, push parent to context");
    // Need a copy (on stack) in case *this is destructed after push.
    std::coroutine_handle child = this->child->self();
    tls::context()->push(std::bit_cast<task_handle>(parent));
    return child;
  }

  frame *child;
  frame *parent;
};

struct call_awaitable : std::suspend_always {

  auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
    LF_LOG("Calling");
    return child->self();
  }

  frame *child;
};

// -------------------------------------------------------------------------------- //

struct join_awaitable {
 private:
  void take_fibre_reset_frame() const noexcept {
    // Steals have happened so we cannot currently own this tasks stack.
    LF_ASSERT(self->load_steals() != 0);
    *tls::fibre() = fibre{self->fibril()};
    // Some steals have happened, need to reset the control block.
    self->reset();
  }

 public:
  auto await_ready() const noexcept -> bool {
    // If no steals then we are the only owner of the parent and we are ready to join.
    if (self->load_steals() == 0) {
      LF_LOG("Sync ready (no steals)");
      // Therefore no need to reset the control block.
      return true;
    }
    // Currently:            joins() = k_u16_max - num_joined
    // Hence:       k_u16_max - joins() = num_joined

    // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
    // better if we see all the decrements to joins() and avoid suspending
    // the coroutine if possible. Cannot fetch_sub() here and write to frame
    // as coroutine must be suspended first.
    auto joined = k_u16_max - self->load_joins(std::memory_order_acquire);

    if (self->load_steals() == joined) {
      LF_LOG("Sync is ready");
      take_fibre_reset_frame();
      return true;
    }

    LF_LOG("Sync not ready");
    return false;
  }

  auto await_suspend(std::coroutine_handle<> task) const noexcept -> std::coroutine_handle<> {
    // Currently        joins  = k_u16_max  - num_joined
    // We set           joins  = joins()    - (k_u16_max - num_steals)
    //                         = num_steals - num_joined

    // Hence               joined = k_u16_max - num_joined
    //         k_u16_max - joined = num_joined

    auto steals = self->load_steals();
    auto joined = self->fetch_sub_joins(k_u16_max - steals, std::memory_order_release);

    if (steals == k_u16_max - joined) {
      // We set joins after all children had completed therefore we can resume the task.
      // Need to acquire to ensure we see all writes by other threads to the result.
      std::atomic_thread_fence(std::memory_order_acquire);
      LF_LOG("Wins join race");
      take_fibre_reset_frame();
      return task;
    }
    LF_LOG("Looses join race");
    // Someone else is responsible for running this task and we have run out of work.
    // We cannot touch *this or deference self as someone may have resumed already!
    // We cannot currently own this stack (checking would violate above).
    return std::noop_coroutine();
  }

  void await_resume() const noexcept {
    LF_LOG("join resumes");
    // Check we have been reset.
    LF_ASSERT(self->load_steals() == 0);
    LF_ASSERT_NO_ASSUME(self->load_joins(std::memory_order_acquire) == k_u16_max);
    LF_ASSERT(self->fibril()->top() == tls::fibre()->top());
  }

  frame *self;
};

} // namespace lf::impl

#endif /* CF3E6AC4_246A_4131_BF7A_FE5CD641A19B */
