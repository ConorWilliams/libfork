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
//        child->is_cancelled() → child not spawned (fork/call via child_scope_ops)
//
//   B. awaitable::await_suspend (Cancel=false):
//        parent.promise().frame.is_cancelled() → child not spawned (fork/call via scope_ops)
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
// B. Cancel=false: parent frame cancellation propagation.
//
//    An inner task receives a stop_source& that IS its own frame's cancel
//    source (bound via child_scope_ops::call_drop / Cancel=true).  It calls
//    request_stop() on it, making its own is_cancelled() return true, then
//    tries to launch sub-tasks via scope_ops (Cancel=false).  Those are
//    skipped because parent.is_cancelled() is true (path B).
//    At join, handle_cancel fires (paths D+E).
// ============================================================

struct inner_call_after_self_cancel {
  template <typename Context>
  static auto operator()(lf::env<Context>, lf::stop_source &my_cancel, std::atomic<int> &count)
      -> lf::task<void, Context> {
    my_cancel.request_stop(); // make this frame's is_cancelled() == true
    auto sc = co_await lf::scope();
    co_await sc.call_drop(count_up_void{}, count); // Cancel=false: parent cancelled → skip
    co_await sc.fork_drop(count_up_void{}, count); // Cancel=false: parent cancelled → skip
    co_await sc.join();                            // paths D+E: join fires handle_cancel
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
    co_await sc.join();   // is_cancelled after child cancels → handle_cancel
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
    co_await sc.join();   // cancelled + exception → handle_cancel drops exception
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
    co_await sc.join();   // cancelled; exceptions dropped
    count.fetch_add(100); // must not be reached
  }
};

template <typename Context>
auto test_sibling_exception_dropped_when_cancelled(lf::env<Context>) -> lf::task<bool, Context> {
  std::atomic<int> count = 0;
  auto outer_sc = co_await lf::child_scope();
  co_await outer_sc.call_drop(inner_sibling_throws_and_cancel{}, outer_sc, count);
  co_await outer_sc.join();
  co_return count.load() >= 2 && count.load() < 100;
}

#endif // LF_COMPILER_EXCEPTIONS

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
    TestType scheduler{thr};
    tests(scheduler);
  }
}
