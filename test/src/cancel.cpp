#include <catch2/catch_test_macros.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// Exhaustive tests for all cancellation paths in promise.cxx.
//
// Cancellation check-points in promise.cxx:
//
//   A. awaitable::await_suspend (Cancel=true):
//        child->is_cancelled() → child not spawned (fork_with/call_with)
//
//   B. awaitable::await_suspend (Cancel=false):
//        parent.promise().frame.is_cancelled() → child not spawned (fork/call)
//
//   C. final_suspend_full / final_suspend_trailing:
//        parent->is_cancelled() after winning join race → exception dropped,
//        iterative ancestor cleanup (exercises concurrent/stolen path)
//
//   D. join_awaitable::await_ready:
//        is_cancelled() forces suspension even when steals==0
//
//   E. join_awaitable::await_suspend:
//        is_cancelled() after winning join race → handle_cancel()
//
//   F. handle_cancel (exception_bit set on cancelled frame):
//        exception dropped, not propagated to caller

namespace {

// ============================================================
// Basic helper tasks
// ============================================================

// Returns the old count (i.e. before incrementing)
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
// A. Cancel=true: child-specific cancellation (call_with / fork_with)
//
//    Exercises awaitable::await_suspend's Cancel=true branch.
//    The check is on the CHILD frame's cancel token.
// ============================================================

// Pre-cancelled call_with_drop: child not run
template <typename Context>
auto test_call_with_drop_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  stop.request_stop();
  co_await sc.call_with_drop(&stop, count_up_void{}, count);
  co_await lf::join();
  co_return count.load() == 0;
}

// Pre-cancelled call_with with return value: return address not written
template <typename Context>
auto test_call_with_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 99;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  stop.request_stop();
  co_await sc.call_with(&stop, &result, count_up{}, count);
  co_await lf::join();
  co_return result == 99 && count.load() == 0;
}

// Pre-cancelled fork_with_drop: child not run
template <typename Context>
auto test_fork_with_drop_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  stop.request_stop();
  co_await sc.fork_with_drop(&stop, count_up_void{}, count);
  co_await lf::join();
  co_return count.load() == 0;
}

// Pre-cancelled fork_with with return value: return address not written
template <typename Context>
auto test_fork_with_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 99;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  stop.request_stop();
  co_await sc.fork_with(&stop, &result, count_up{}, count);
  co_await lf::join();
  co_return result == 99 && count.load() == 0;
}

// Positive: call_with NOT cancelled - child runs
template <typename Context>
auto test_call_with_not_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 0;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.call_with(&stop, &result, count_up{}, count);
  co_await lf::join();
  co_return result == 0 && count.load() == 1;
}

// Positive: fork_with NOT cancelled - child runs
template <typename Context>
auto test_fork_with_not_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  int result = 0;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.fork_with(&stop, &result, count_up{}, count);
  co_await lf::join();
  co_return result == 0 && count.load() == 1;
}

// Multiple fork_with_drop: all pre-cancelled, none run
template <typename Context>
auto test_multiple_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto stop = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  stop.request_stop();
  co_await sc.fork_with_drop(&stop, count_up_void{}, count);
  co_await sc.fork_with_drop(&stop, count_up_void{}, count);
  co_await sc.fork_with_drop(&stop, count_up_void{}, count);
  co_await lf::join();
  co_return count.load() == 0;
}

// Mixed: some children have a cancelled token, others don't.
// Only the non-cancelled children should run.
template <typename Context>
auto test_mixed_cancel(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto stop_run = co_await lf::child_stop_source();
  auto stop_skip = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  stop_skip.request_stop();
  co_await sc.fork_with_drop(&stop_run, count_up_void{}, count);  // runs
  co_await sc.fork_with_drop(&stop_skip, count_up_void{}, count); // skipped
  co_await sc.fork_with_drop(&stop_run, count_up_void{}, count);  // runs
  co_await lf::join();
  co_return count.load() == 2;
}

// ============================================================
// B. Cancel=false: parent frame cancellation propagation
//
//    Exercises awaitable::await_suspend's Cancel=false branch.
//    The check is on the PARENT frame's is_cancelled().
//
//    Strategy: use call_with to give an inner task a specific stop source as
//    its frame.cancel. The inner task receives that pointer as an argument,
//    calls request_stop() on it (making its own is_cancelled() true), then
//    tries to launch sub-tasks via the no-cancel (Cancel=false) API.
//
//    The sub-tasks are skipped because parent.is_cancelled() is true.
//    At the subsequent join, handle_cancel (path E/D) fires, cleans up the
//    inner task, and resumes the outer task normally.
//
//    Outer task's stop chain does NOT include the inner task's cs, so the
//    outer task completes normally and returns the count comparison.
// ============================================================

// Inner task: cancels its own stop source, then tries call_drop (Cancel=false)
struct inner_call_after_self_cancel {
  template <typename Context, typename CS>
  static auto
  operator()(lf::env<Context>, CS *my_cancel, std::atomic<int> &count) -> lf::task<void, Context> {
    my_cancel->request_stop(); // Make this frame's is_cancelled() return true
    auto sc = co_await lf::scope();
    // Cancel=false: parent (this frame) is_cancelled() → child not spawned (path B)
    co_await sc.call_drop(count_up_void{}, count);
    // Cancel=false: same check for fork (path B)
    co_await sc.fork_drop(count_up_void{}, count);
    // Paths D+E: join sees is_cancelled(), fires handle_cancel, outer task resumes
    co_await lf::join();
    count.fetch_add(100); // must not be reached
  }
};

// test_call_parent_cancel: outer wraps inner via call_with so inner.frame.cancel = &cs.
// inner cancels cs and verifies both call_drop and fork_drop are skipped.
template <typename Context>
auto test_call_parent_cancel(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto cs = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.call_with_drop(&cs, inner_call_after_self_cancel{}, &cs, count);
  co_await lf::join();
  co_return count.load() == 0;
}

// ============================================================
// C/D/E. Concurrent cancellation: final_suspend + join interaction
//
//    A forked child cancels the parent's stop source, then the parent task
//    arrives at join. The join detects cancellation (paths D+E) and calls
//    handle_cancel. With multiple threads (busy pool), the cancel may also
//    be observed in final_suspend_full (path C).
// ============================================================

// A child task that cancels a stop source then runs normally.
// Template on CS to avoid naming lf::stop_source directly.
struct cancel_cs {
  template <typename Context, typename CS>
  static auto operator()(lf::env<Context>, CS *cs, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1);
    cs->request_stop();
    co_return;
  }
};

// Outer task: forked children cancel the frame's stop source, then join
// detects cancel (D+E). The outer is NOT the cancelled task - it just
// verifies the inner completes cleanly.
struct inner_fork_then_cancel_at_join {
  template <typename Context, typename CS>
  static auto
  operator()(lf::env<Context>, CS *my_cancel, std::atomic<int> &count) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(cancel_cs{}, my_cancel, count);
    co_await lf::join();  // is_cancelled after child cancels cs → handle_cancel
    count.fetch_add(100); // must not be reached
  }
};

template <typename Context>
auto test_fork_cancel_at_join(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto cs = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.call_with_drop(&cs, inner_fork_then_cancel_at_join{}, &cs, count);
  co_await lf::join();
  co_return count.load() == 1; // cancel_cs ran exactly once
}

// ============================================================
// F. Exception + cancellation interaction
//
//    When a frame is cancelled at join time and the frame has an exception
//    stashed, handle_cancel drops (not propagates) the exception.
// ============================================================

#if LF_COMPILER_EXCEPTIONS

// A task that throws unconditionally
struct just_throw {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    throw std::runtime_error("test exception");
    co_return;
  }
};

// Inner task: forks just_throw, then rethrows at join.
// Used to verify exceptions propagate when no cancellation.
struct inner_forks_throwing {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(just_throw{});
    co_await lf::join(); // not cancelled → exception_bit=1 → rethrow (path await_resume)
    co_return;           // not reached
  }
};

// Test F1: exception propagates through join and all the way to recv.get()
// when the task is NOT cancelled.
template <typename Context>
auto test_exception_propagates(lf::env<Context>) -> lf::task<void, Context> {
  auto cs = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.call_with_drop(&cs, inner_forks_throwing{});
  co_await lf::join();
}

// A child task that cancels a stop source AND throws.
// Exception stashes in its parent (inner_cancel_and_throw's frame).
struct cancel_cs_and_throw {
  template <typename Context, typename CS>
  static auto operator()(lf::env<Context>, CS *cs, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1); // Confirm this task ran
    cs->request_stop(); // Cancel parent's stop source
    throw std::runtime_error("should be dropped");
    co_return;
  }
};

// Inner task:
//   1. fork cancel_cs_and_throw → child cancels my_cancel AND stashes exception
//      in this frame (not outer's).
//   2. At join: is_cancelled AND exception_bit → handle_cancel drops exception (path F),
//      then final_suspend_leading resumes outer.
// Outer sees no exception and count==1.
struct inner_cancel_and_throw {
  template <typename Context, typename CS>
  static auto
  operator()(lf::env<Context>, CS *my_cancel, std::atomic<int> &count) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(cancel_cs_and_throw{}, my_cancel, count);
    co_await lf::join();  // cancelled + exception → handle_cancel drops exception
    count.fetch_add(100); // must not be reached
  }
};

// Test F2: exception stashed in a cancelled frame is silently dropped.
// recv.get() does NOT throw; cancel_cs_and_throw ran (count==1).
template <typename Context>
auto test_exception_dropped_when_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto cs = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.call_with_drop(&cs, inner_cancel_and_throw{}, &cs, count);
  co_await lf::join(); // outer is NOT cancelled, no exception reaches here
  co_return count.load() == 1;
}

// Test F3: combined - verify that a non-cancellation exception still propagates
// when a sibling child cancelled the frame BUT the throwing child ran first
// (i.e., the exception stash/drop is frame-local, not task-global).
//
// inner_throws_first_then_cancel:
//   fork throw_child  → exception stashed in this frame
//   fork cancel_child → cancels my_cancel
//   join → cancelled + exception → exception dropped
struct just_throw_and_count {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count) -> lf::task<void, Context> {
    count.fetch_add(1);
    throw std::runtime_error("sibling exception");
    co_return;
  }
};

struct inner_sibling_throws_and_cancel {
  template <typename Context, typename CS>
  static auto
  operator()(lf::env<Context>, CS *my_cancel, std::atomic<int> &count) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(just_throw_and_count{}, count);
    co_await sc.fork_drop(cancel_cs{}, my_cancel, count);
    co_await lf::join();  // cancelled; any exceptions dropped
    count.fetch_add(100); // must not be reached
  }
};

template <typename Context>
auto test_sibling_exception_dropped_when_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto cs = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();
  co_await sc.call_with_drop(&cs, inner_sibling_throws_and_cancel{}, &cs, count);
  co_await lf::join(); // outer is NOT cancelled, no exception
  // just_throw_and_count and cancel_cs both ran → count >= 2
  co_return count.load() >= 2 && count.load() < 100;
}

#endif // LF_COMPILER_EXCEPTIONS

// ============================================================
// Run all tests against a given scheduler
// ============================================================

template <typename Sch>
void tests(Sch &scheduler) {
  using Ctx = lf::context_t<Sch>;

  // A. Cancel=true (child-specific token)

  SECTION("call_with_drop: pre-cancelled child is not run") {
    auto recv = schedule(scheduler, test_call_with_drop_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("call_with: pre-cancelled child is not run, return address not written") {
    auto recv = schedule(scheduler, test_call_with_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork_with_drop: pre-cancelled child is not run") {
    auto recv = schedule(scheduler, test_fork_with_drop_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork_with: pre-cancelled child is not run, return address not written") {
    auto recv = schedule(scheduler, test_fork_with_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("call_with: positive - not cancelled, child runs and writes result") {
    auto recv = schedule(scheduler, test_call_with_not_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork_with: positive - not cancelled, child runs and writes result") {
    auto recv = schedule(scheduler, test_fork_with_not_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("multiple fork_with_drop: all pre-cancelled, none run") {
    auto recv = schedule(scheduler, test_multiple_cancelled<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork_with_drop: mixed tokens - only non-cancelled children run") {
    auto recv = schedule(scheduler, test_mixed_cancel<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  // B/D/E. Cancel=false (parent frame propagation) + join cancel handling

  SECTION("call_drop/fork_drop (Cancel=false): skipped when parent frame is cancelled; "
          "join fires handle_cancel") {
    auto recv = schedule(scheduler, test_call_parent_cancel<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  SECTION("fork child cancels parent stop source; join detects cancel via handle_cancel") {
    auto recv = schedule(scheduler, test_fork_cancel_at_join<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

#if LF_COMPILER_EXCEPTIONS

  // F. Exception + cancellation

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
    TestType scheduler{thr};
    tests(scheduler);
  }
}
