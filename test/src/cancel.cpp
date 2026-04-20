#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// Exhaustive tests for all stop-token paths in promise.cxx.
//
// Stop check-points in promise.cxx:
//
//   A. awaitable::await_suspend (StopToken=true):
//        child->stop_requested() → child not spawned (fork/call via child_scope_ops)
//
//   B. awaitable::await_suspend (StopToken=false):
//        parent.promise().frame.stop_requested() → child not spawned (fork/call via scope_ops)
//
//   C. final_suspend_full / final_suspend_trailing:
//        parent->stop_requested() after winning join race → exception dropped,
//        iterative ancestor cleanup (exercises concurrent/stolen path)
//
//   D. join_awaitable::await_ready:
//        stop_requested() forces suspension even when steals==0
//
//   E. join_awaitable::await_suspend:
//        stop_requested() after winning join race → handle_stop()
//
//   F. handle_stop (exception_bit set on stopped frame):
//        exception dropped, not propagated to caller
//
//   G. Nested child_scope chain propagation:
//        inner child_scope inherits parent's stop token; stopping the outer
//        source propagates through the chain to the inner scope.
//
//   H. Stoppable receiver / pre-cancelled root:
//        recv_state<T, true> + receiver::request_stop() immediately after
//        schedule() — covers the goto-cleanup fast path in root.cxx on
//        schedulers where the task has not yet begun running.  Racy in
//        principle, so the test only asserts completion, not that the body
//        was skipped.
//
//   I. Stress tests: concurrent cancellation under contention.

namespace {

// ============================================================
// Basic helper tasks
// ============================================================

struct count_up {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count) -> lf::task<int, Context> {
    co_return count.fetch_add(1);
  }
};

struct count_up_void {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1);
    co_return;
  }
};

// ============================================================
// A. Cancel=true: child-specific cancellation via child_scope_ops.
//
//    child_scope_ops binds its stop_source as Cancel=true on every fork/call.
//    Calling sc.request_stop() before launching exercises
//    awaitable::await_suspend's Cancel=true branch.
// ============================================================

template <typename Context>
auto test_call_drop_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto sc = co_await lf::child_scope();
  sc.request_stop();
  co_await sc.call_drop(count_up_void{}, count);
  co_await sc.join();
  co_return count.load() == 0;
}

template <typename Context>
auto test_call_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 99;
  auto sc = co_await lf::child_scope();
  sc.request_stop();
  co_await sc.call(&result, count_up{}, count);
  co_await sc.join();
  co_return result == 99 && count.load() == 0;
}

template <typename Context>
auto test_fork_drop_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto sc = co_await lf::child_scope();
  sc.request_stop();
  co_await sc.fork_drop(count_up_void{}, count);
  co_await sc.join();
  co_return count.load() == 0;
}

template <typename Context>
auto test_fork_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 99;
  auto sc = co_await lf::child_scope();
  sc.request_stop();
  co_await sc.fork(&result, count_up{}, count);
  co_await sc.join();
  co_return result == 99 && count.load() == 0;
}

template <typename Context>
auto test_call_not_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 0;
  auto sc = co_await lf::child_scope();
  co_await sc.call(&result, count_up{}, count);
  co_await sc.join();
  co_return result == 0 && count.load() == 1;
}

template <typename Context>
auto test_fork_not_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 0;
  auto sc = co_await lf::child_scope();
  co_await sc.fork(&result, count_up{}, count);
  co_await sc.join();
  co_return result == 0 && count.load() == 1;
}

template <typename Context>
auto test_multiple_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto sc = co_await lf::child_scope();
  sc.request_stop();
  co_await sc.fork_drop(count_up_void{}, count);
  co_await sc.fork_drop(count_up_void{}, count);
  co_await sc.fork_drop(count_up_void{}, count);
  co_await sc.join();
  co_return count.load() == 0;
}

template <typename Context>
auto test_mixed_cancel(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto sc_run = co_await lf::child_scope();
  auto sc_skip = co_await lf::child_scope();
  sc_skip.request_stop();
  co_await sc_run.fork_drop(count_up_void{}, count);  // runs
  co_await sc_skip.fork_drop(count_up_void{}, count); // skipped
  co_await sc_run.fork_drop(count_up_void{}, count);  // runs
  co_await sc_run.join();
  co_await sc_skip.join();
  co_return count.load() == 2;
}

// ============================================================
// B. StopToken=false: parent frame stop propagation.
//
//    An inner task receives a stop_source& that IS its own frame's stop
//    source (bound via child_scope_ops::call_drop / StopToken=true).  It calls
//    request_stop() on it, making its own stop_requested() return true, then
//    tries to launch sub-tasks via scope_ops (StopToken=false).  Those are
//    skipped because parent.frame.stop_requested() is true (path B).
//    At join, handle_stop fires (paths D+E).
// ============================================================

struct inner_call_after_self_cancel {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &my_cancel, std::atomic<int> &count)
      -> lf::task<void, Context> {
    my_cancel.request_stop(); // make this frame's stop_requested() == true
    auto sc = co_await lf::scope();
    co_await sc.call_drop(count_up_void{}, count); // StopToken=false: stop requested → skip
    co_await sc.fork_drop(count_up_void{}, count); // StopToken=false: stop requested → skip
    co_await sc.join();                            // paths D+E: join fires handle_stop
    count.fetch_add(100);                          // must not be reached
  }
};

template <typename Context>
auto test_call_parent_stop_source(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto outer_sc = co_await lf::child_scope();
  // Pass the scope's stop_source by reference so the inner task can cancel it.
  co_await outer_sc.call_drop(inner_call_after_self_cancel{}, outer_sc, count);
  co_await outer_sc.join();
  co_return count.load() == 0;
}

// ============================================================
// C/D/E. Concurrent cancellation: final_suspend + join interaction.
// ============================================================

// A child task that cancels a stop_source then completes normally.
struct cancel_source {
  template <typename Context>
  static auto
  operator()(lf::env<Context>, lf::stop_source &src, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1);
    src.request_stop();
    co_return;
  }
};

struct inner_fork_then_cancel_at_join {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &my_cancel, std::atomic<int> &count)
      -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(cancel_source{}, my_cancel, count);
    co_await sc.join();   // stop_requested() after child requests stop → handle_stop
    count.fetch_add(100); // must not be reached
  }
};

template <typename Context>
auto test_fork_cancel_at_join(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto outer_sc = co_await lf::child_scope();
  co_await outer_sc.call_drop(inner_fork_then_cancel_at_join{}, outer_sc, count);
  co_await outer_sc.join();
  co_return count.load() == 1;
}

// ============================================================
// F. Exception + cancellation interaction.
// ============================================================

#if LF_COMPILER_EXCEPTIONS

struct just_throw {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    throw std::runtime_error("test exception");
    co_return;
  }
};

struct inner_forks_throwing {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(just_throw{});
    co_await sc.join(); // not cancelled → rethrow
    co_return;
  }
};

template <typename Context>
auto test_exception_propagates(lf::env<Context>) -> lf::task<void, Context> {
  auto outer_sc = co_await lf::child_scope();
  co_await outer_sc.call_drop(inner_forks_throwing{});
  co_await outer_sc.join();
}

struct cancel_source_and_throw {
  template <typename Context>
  static auto
  operator()(lf::env<Context>, lf::stop_source &src, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1);
    src.request_stop();
    throw std::runtime_error("should be dropped");
    co_return;
  }
};

struct inner_cancel_and_throw {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &my_cancel, std::atomic<int> &count)
      -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(cancel_source_and_throw{}, my_cancel, count);
    co_await sc.join();   // stop requested + exception → handle_stop drops exception
    count.fetch_add(100); // must not be reached
  }
};

template <typename Context>
auto test_exception_dropped_when_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto outer_sc = co_await lf::child_scope();
  co_await outer_sc.call_drop(inner_cancel_and_throw{}, outer_sc, count);
  co_await outer_sc.join();
  co_return count.load() == 1;
}

struct just_throw_and_count {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1);
    throw std::runtime_error("sibling exception");
    co_return;
  }
};

struct inner_sibling_throws_and_cancel {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &my_cancel, std::atomic<int> &count)
      -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(just_throw_and_count{}, count);
    co_await sc.fork_drop(cancel_source{}, my_cancel, count);
    co_await sc.join();   // stop requested; exceptions dropped
    count.fetch_add(100); // must not be reached
  }
};

template <typename Context>
auto test_sibling_exception_dropped_when_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto outer_sc = co_await lf::child_scope();
  co_await outer_sc.call_drop(inner_sibling_throws_and_cancel{}, outer_sc, count);
  co_await outer_sc.join();
  auto c = count.load();
  co_return c >= 2 && c < 100;
}

#endif // LF_COMPILER_EXCEPTIONS

// ============================================================
// G. Nested child_scope chain propagation.
//
//    A child_scope created inside a task that runs under another child_scope
//    has m_parent pointing to the outer scope's stop_source.  Stopping the
//    outer source propagates through the chain, making the inner scope's
//    stop_requested() return true (path A).
// ============================================================

struct inner_with_nested_scope {
  template <typename Context>
  static auto
  operator()(lf::env<Context>, lf::stop_source &outer, std::atomic<int> &count) -> lf::task<void, Context> {
    auto inner_sc = co_await lf::child_scope();
    // Cancel the outer scope; inner_sc.m_parent == &outer, so the chain fires.
    outer.request_stop();
    co_await inner_sc.fork_drop(count_up_void{}, count); // skipped: inner_sc is stopped
    co_await inner_sc.join();                            // handle_stop
    count.fetch_add(100);                                // must not be reached
  }
};

template <typename Context>
auto test_nested_child_scope_chain(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto outer_sc = co_await lf::child_scope();
  co_await outer_sc.call_drop(inner_with_nested_scope{}, outer_sc, count);
  co_await outer_sc.join();
  co_return count.load() == 0;
}

// ============================================================
// H. Stoppable receiver / pre-cancelled root.
//
//    Using recv_state<T, true> + receiver::request_stop() exercises the
//    goto-cleanup fast path in root.cxx when stop is requested before the
//    worker resumes the task.
// ============================================================

template <typename Context>
auto pre_cancelled_root_fn(lf::env<Context>, std::atomic<bool> *ran) -> lf::task<void, Context> {
  ran->store(true, std::memory_order_relaxed);
  co_return;
}

// ============================================================
// I. Stress tests: concurrent cancellation under contention.
//
//    These tests fork many tasks across multiple threads to maximize the
//    probability of hitting the concurrent paths in final_suspend_full,
//    final_suspend_trailing, and join_awaitable::await_suspend with
//    stop_requested() == true.
// ============================================================

// --- Leaf task: does a tiny amount of work then returns.
struct leaf_work {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1, std::memory_order_relaxed);
    co_return;
  }
};

// --- Fan-out many forks, one sibling cancels the scope mid-flight.
//
//     With enough forks and threads, some children will be in-flight when
//     stop fires. This exercises:
//       - final_suspend_trailing: child completes, wins join race, sees stop
//       - final_suspend_full: iterative ancestor climb on stopped frames
//       - awaitable::await_suspend: children launched after stop are skipped
//       - join_awaitable: stop detected at join with steals > 0

struct stress_fan_cancel_inner {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &my_stop, std::atomic<int> &count, int width)
      -> lf::task<void, Context> {
    auto sc = co_await lf::scope();

    // Fork width children; the last one cancels this scope.
    for (int i = 0; i < width; ++i) {
      if (i == width / 2) {
        co_await sc.fork_drop(cancel_source{}, my_stop, count);
      } else {
        co_await sc.fork_drop(leaf_work{}, count);
      }
    }
    co_await sc.join();
    // Should not be reached — cancel_source fired mid-fan.
    count.fetch_add(10000, std::memory_order_relaxed);
  }
};

template <typename Context>
auto stress_fan_cancel(lf::env<Context>, int width) -> lf::task<void, Context> {
  std::atomic<int> count = 0;
  auto outer = co_await lf::child_scope();
  co_await outer.call_drop(stress_fan_cancel_inner{}, outer, count, width);
  co_await outer.join();
  // Unreachable: outer scope was cancelled by the inner task.
  std::terminate();
}

// --- Deep recursive fork tree with cancellation at a specific depth.
//
//     This creates a binary tree of forks. When a node at the target depth
//     fires, it cancels the scope. This stresses:
//       - final_suspend_full loop: many frames in the ancestor chain may be
//         stopped, causing iterative climbing
//       - final_suspend_trailing: stolen forks completing concurrently
//       - Stack ownership transfer under cancellation

struct tree_cancel_node {
  template <typename Context>
  static auto
  operator()(lf::env<Context>, lf::stop_source &root_stop, std::atomic<int> &count, int depth, int cancel_at)
      -> lf::task<void, Context> {
    count.fetch_add(1, std::memory_order_relaxed);

    if (depth <= 0) {
      co_return;
    }

    if (depth == cancel_at) {
      root_stop.request_stop();
      co_return;
    }

    auto sc = co_await lf::scope();
    co_await sc.fork_drop(tree_cancel_node{}, root_stop, count, depth - 1, cancel_at);
    co_await sc.fork_drop(tree_cancel_node{}, root_stop, count, depth - 1, cancel_at);
    co_await sc.join();
  }
};

template <typename Context>
auto stress_tree_cancel(lf::env<Context>, int depth, int cancel_at) -> lf::task<void, Context> {
  std::atomic<int> count = 0;
  auto outer = co_await lf::child_scope();
  co_await outer.call_drop(tree_cancel_node{}, outer, count, depth, cancel_at);
  co_await outer.join();
  // Unreachable: outer scope was cancelled by a tree node.
  std::terminate();
}

// --- Repeated schedule + cancel: exercises root.cxx stop path and receiver.
//
//     Rapidly schedules tasks and immediately cancels them. The race between
//     the worker picking up the task and the cancellation request stresses
//     the root_pkg pre-cancelled path and final_suspend from root frames.

struct busy_leaf {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    co_return;
  }
};

// --- Many-fork cancel with nested child_scopes at multiple levels.
//
//     An outer child_scope forks N tasks. Each inner task creates its own
//     child_scope and forks M children. Mid-way, the outer scope is cancelled.
//     This tests chain propagation under concurrent fork completion, hitting
//     final_suspend_full's iterative climb through multiple nested stopped
//     frames.

struct nested_inner_worker {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count, int width) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    for (int i = 0; i < width; ++i) {
      co_await sc.fork_drop(leaf_work{}, count);
    }
    co_await sc.join();
  }
};

struct nested_cancel_orchestrator {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &root_stop, std::atomic<int> &count, int width)
      -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    for (int i = 0; i < width; ++i) {
      if (i == width / 2) {
        // Cancel after forking half the work
        root_stop.request_stop();
      }
      co_await sc.fork_drop(nested_inner_worker{}, count, width);
    }
    co_await sc.join();
    // Should not be reached
    count.fetch_add(100000, std::memory_order_relaxed);
  }
};

template <typename Context>
auto stress_nested_cancel(lf::env<Context>, int width) -> lf::task<void, Context> {
  std::atomic<int> count = 0;
  auto outer = co_await lf::child_scope();
  co_await outer.call_drop(nested_cancel_orchestrator{}, outer, count, width);
  co_await outer.join();
  // Unreachable: outer scope was cancelled by the orchestrator.
  std::terminate();
}

// ============================================================
// Run all tests against a given scheduler
// ============================================================

template <typename Sch>
void tests(Sch &scheduler) {

  using Ctx = lf::context_t<Sch>;

  SECTION("call_drop: pre-cancelled child is not run") {
    auto recv = schedule(scheduler, test_call_drop_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("call: pre-cancelled child is not run, return address not written") {
    auto recv = schedule(scheduler, test_call_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork_drop: pre-cancelled child is not run") {
    auto recv = schedule(scheduler, test_fork_drop_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork: pre-cancelled child is not run, return address not written") {
    auto recv = schedule(scheduler, test_fork_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("call: positive - not cancelled, child runs and writes result") {
    auto recv = schedule(scheduler, test_call_not_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork: positive - not cancelled, child runs and writes result") {
    auto recv = schedule(scheduler, test_fork_not_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("multiple fork_drops: all pre-cancelled, none run") {
    auto recv = schedule(scheduler, test_multiple_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("mixed scopes: only non-cancelled children run") {
    auto recv = schedule(scheduler, test_mixed_cancel<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("call_drop/fork_drop: skipped when parent frame is cancelled; join fires handle_cancel") {
    auto recv = schedule(scheduler, test_call_parent_stop_source<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork child cancels parent stop source; join detects cancel via handle_cancel") {
    auto recv = schedule(scheduler, test_fork_cancel_at_join<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("nested child_scope: stopping outer scope propagates to inner via chain") {
    auto recv = schedule(scheduler, test_nested_child_scope_chain<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("stoppable receiver: recv_state + request_stop completes cleanly") {
    std::atomic<bool> ran = false;
    lf::recv_state<void, true> state;
    auto recv = lf::schedule(scheduler, std::move(state), pre_cancelled_root_fn<Ctx>, &ran);
    REQUIRE(recv.valid());
    recv.stop_source().request_stop();

#if LF_COMPILER_EXCEPTIONS
    REQUIRE_THROWS_AS(std::move(recv).get(), lf::operation_cancelled_error);
#else
    recv.wait();
#endif

    // The task body may or may not have run depending on scheduler timing;
    // what matters is that get() completes without error.
    std::ignore = ran.load();
  }

  // --- Stress tests (paths C/D/E under contention) ---

  SECTION("stress: fan-out cancel, width=16") {
    auto recv = schedule(scheduler, stress_fan_cancel<Ctx>, 16);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: fan-out cancel, width=64") {
    auto recv = schedule(scheduler, stress_fan_cancel<Ctx>, 64);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: tree cancel depth=6, cancel at depth=3") {
    auto recv = schedule(scheduler, stress_tree_cancel<Ctx>, 6, 3);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: tree cancel depth=8, cancel at depth=1 (near leaf)") {
    auto recv = schedule(scheduler, stress_tree_cancel<Ctx>, 8, 1);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: tree cancel depth=8, cancel at depth=7 (near root)") {
    auto recv = schedule(scheduler, stress_tree_cancel<Ctx>, 8, 7);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: nested child_scope cancel, width=8") {
    auto recv = schedule(scheduler, stress_nested_cancel<Ctx>, 8);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: nested child_scope cancel, width=32") {
    auto recv = schedule(scheduler, stress_nested_cancel<Ctx>, 32);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("stress: rapid schedule + cancel") {
    for (int i = 0; i < 50; ++i) {
      lf::recv_state<void, true> state;
      auto recv = lf::schedule(scheduler, std::move(state), busy_leaf{});
      recv.stop_source().request_stop();
#if LF_COMPILER_EXCEPTIONS
      REQUIRE_THROWS_AS(std::move(recv).get(), lf::operation_cancelled_error);
#else
      recv.wait();
#endif
    }
  }

#if LF_COMPILER_EXCEPTIONS

  SECTION("exception propagates through join when frame is NOT cancelled") {
    auto recv = schedule(scheduler, test_exception_propagates<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
  }

  SECTION("exception in cancelled frame is dropped by handle_cancel; recv.get() does not throw") {
    auto recv = schedule(scheduler, test_exception_dropped_when_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("sibling exception dropped when another sibling cancels the frame") {
    auto recv = schedule(scheduler, test_sibling_exception_dropped_when_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

#endif // LF_COMPILER_EXCEPTIONS
}

using mono_inline_ctx = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector>;
using poly_inline_ctx = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector>;

} // namespace

TEMPLATE_TEST_CASE("Inline cancel", "[cancel]", mono_inline_ctx, poly_inline_ctx) {
  lf::inline_scheduler<TestType> sch{};
  tests(sch);
}

namespace {

using mono_busy_thread_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_busy_thread_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

} // namespace

TEMPLATE_TEST_CASE("Busy cancel", "[cancel]", mono_busy_thread_pool, poly_busy_thread_pool) {

  STATIC_REQUIRE(lf::scheduler<TestType>);

  for (std::size_t thr = 1; thr < 4; ++thr) {
    DYNAMIC_SECTION("threads=" << thr) {
      TestType scheduler{thr};
      tests(scheduler);
    }
  }
}

namespace {

// Stress tests repeated at higher thread counts to maximize contention.
template <typename Sch>
void stress_tests(Sch &scheduler) {
  using Ctx = lf::context_t<Sch>;

  SECTION("stress: repeated fan cancel") {
    for (int rep = 0; rep < 20; ++rep) {
      auto recv = schedule(scheduler, stress_fan_cancel<Ctx>, 32);
      REQUIRE(recv.valid());
      std::move(recv).get();
    }
  }

  SECTION("stress: repeated tree cancel") {
    for (int rep = 0; rep < 20; ++rep) {
      auto recv = schedule(scheduler, stress_tree_cancel<Ctx>, 7, 3);
      REQUIRE(recv.valid());
      std::move(recv).get();
    }
  }

  SECTION("stress: repeated nested cancel") {
    for (int rep = 0; rep < 20; ++rep) {
      auto recv = schedule(scheduler, stress_nested_cancel<Ctx>, 16);
      REQUIRE(recv.valid());
      std::move(recv).get();
    }
  }
}

} // namespace

TEMPLATE_TEST_CASE("Busy cancel stress", "[cancel][stress]", mono_busy_thread_pool, poly_busy_thread_pool) {

  STATIC_REQUIRE(lf::scheduler<TestType>);

  std::size_t max_thr = std::min(8UZ, static_cast<std::size_t>(std::thread::hardware_concurrency()));

  for (std::size_t thr = 2; thr <= max_thr; thr *= 2) {
    DYNAMIC_SECTION("threads=" << thr) {
      TestType scheduler{thr};
      stress_tests(scheduler);
    }
  }
}
