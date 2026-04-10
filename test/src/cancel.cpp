#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// ============================================================
//  Helpers
// ============================================================

namespace {

using lf::cancellation;
using lf::env;
using lf::task;

// ---- context aliases ----

using mono_inline_ctx = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector>;
using poly_inline_ctx = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector>;
using mono_busy_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_busy_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

// ---- leaf tasks ----

template <typename Context>
auto noop_task(env<Context>) -> task<void, Context> {
  co_return;
}

template <typename Context>
auto counting_task(env<Context>, std::atomic<int> *counter) -> task<void, Context> {
  counter->fetch_add(1, std::memory_order_relaxed);
  co_return;
}

template <typename Context>
auto returning_task(env<Context>) -> task<int, Context> {
  co_return 99;
}

// Signals cancellation, then returns.
template <typename Context>
auto signal_cancel_task(env<Context>, cancellation *tok) -> task<void, Context> {
  tok->request_stop();
  co_return;
}

// ============================================================
//  Tasks for "pre-cancel: child task runs, grandchild is skipped"
//
//  outer_with_cancel  [cancel=nullptr]
//    → scope::call(&tok, inner_with_cancel)
//        inner_with_cancel  [cancel=&tok]
//          → scope::call(counting_task)   ← await_transform checks inner's cancel
//                                           → skipped when tok is stopped
// ============================================================

template <typename Context>
auto inner_with_cancel(env<Context>, std::atomic<int> *grandchild_ran) -> task<void, Context> {
  using S = lf::scope<Context>;
  // Inherited cancel is checked here. If our cancel chain is stopped,
  // counting_task is never created.
  co_await S::call(counting_task<Context>, grandchild_ran);
  co_return;
}

template <typename Context>
auto outer_with_cancel(env<Context>, cancellation *tok, std::atomic<int> *grandchild_ran)
    -> task<void, Context> {
  using S = lf::scope<Context>;
  // Root's cancel=nullptr → await_transform succeeds, creates inner_with_cancel
  // and binds it to tok.
  co_await S::call(tok, inner_with_cancel<Context>, grandchild_ran);
  co_return;
}

// ============================================================
//  Tasks for "fork: child signals cancel, post-join code unreachable"
//
//  fork_outer  [cancel=nullptr]
//    → scope::call(&tok, fork_signal_join)
//        fork_signal_join  [cancel=&tok]
//          → scope::fork(signal_cancel_task)  [cancel inherited=&tok]
//          → co_await join()
//                 join: steals=0, is_cancelled()=true
//                       await_ready → false
//                       await_suspend: steals=0 special-case (bug-fix path)
//                       → handle_cancel → final_suspend cascade
//          [post-join code unreachable]
// ============================================================

template <typename Context>
auto fork_signal_join(env<Context>, cancellation *tok, std::atomic<int> *post_join_ran)
    -> task<void, Context> {
  using S = lf::scope<Context>;

  // Fork a child that sets tok.stop = 1. tok is not yet stopped, so the
  // child is created (parent's await_transform sees is_cancelled()=false).
  co_await S::fork(signal_cancel_task<Context>, tok);

  // Join: with the inline scheduler there are no steals (steals==0).
  // tok is now stopped, so is_cancelled() returns true.
  // await_ready returns false → await_suspend is called.
  // The steals==0 special-case in await_suspend handles this correctly.
  co_await lf::join();

  // Should NOT be reached — cancellation cascades before here.
  post_join_ran->fetch_add(1, std::memory_order_relaxed);
  co_return;
}

template <typename Context>
auto fork_outer(env<Context>, cancellation *tok, std::atomic<int> *post_join_ran) -> task<void, Context> {
  using S = lf::scope<Context>;
  co_await S::call(tok, fork_signal_join<Context>, tok, post_join_ran);
  co_return;
}

// ============================================================
//  Tasks for "second fork skipped when token already stopped"
//
//  fork_two_outer  [cancel=nullptr]
//    → scope::call(&tok, fork_two_children)
//        fork_two_children  [cancel=&tok]
//          → fork(signal_cancel_task)  ← sets tok, completes inline
//          → fork(counting_task)       ← await_transform: is_cancelled()=true → skipped
//          → join()
// ============================================================

template <typename Context>
auto fork_two_children(env<Context>, cancellation *tok, std::atomic<int> *second_ran) -> task<void, Context> {
  using S = lf::scope<Context>;

  co_await S::fork(signal_cancel_task<Context>, tok);
  // After inline child completes tok is stopped.
  // This task's cancel = tok → next await_transform checks is_cancelled() → true → skipped.
  co_await S::fork(counting_task<Context>, second_ran);
  co_await lf::join();
  co_return;
}

template <typename Context>
auto fork_two_outer(env<Context>, cancellation *tok, std::atomic<int> *second_ran) -> task<void, Context> {
  using S = lf::scope<Context>;
  co_await S::call(tok, fork_two_children<Context>, tok, second_ran);
  co_return;
}

// ============================================================
//  Tasks for "return value is default-initialised when cancelled"
//
//  The inner task cancels via a fork, then cascades before co_return 99.
//  The outer task writes the (unset) return value to val, which stays 0.
// ============================================================

template <typename Context>
auto inner_returning(env<Context>, cancellation *tok) -> task<int, Context> {
  using S = lf::scope<Context>;
  co_await S::fork(signal_cancel_task<Context>, tok);
  co_await lf::join(); // cascade happens here
  co_return 99;        // unreachable
}

template <typename Context>
auto outer_returning(env<Context>, cancellation *tok) -> task<int, Context> {
  using S = lf::scope<Context>;
  int val = 0;
  // Call inner with tok in its cancel chain; write result to val.
  co_await S::call(tok, &val, inner_returning<Context>, tok);
  co_return val;
}

#if LF_COMPILER_EXCEPTIONS

// ============================================================
//  Tasks for "exception in forked child that also signals cancel"
//
//  The child throws AND signals cancellation.  Because the parent frame is
//  cancelled, handle_cancel() discards the stashed exception (std::ignore =
//  extract_exception).  The receiver must complete without throwing.
// ============================================================

template <typename Context>
auto throw_and_cancel(env<Context>, cancellation *tok) -> task<void, Context> {
  tok->request_stop();
  LF_THROW(std::runtime_error{"intentional"});
  co_return;
}

template <typename Context>
auto fork_throw_and_cancel(env<Context>, cancellation *tok) -> task<void, Context> {
  using S = lf::scope<Context>;
  co_await S::fork(throw_and_cancel<Context>, tok);
  co_await lf::join();
  co_return;
}

template <typename Context>
auto fork_throw_outer(env<Context>, cancellation *tok) -> task<void, Context> {
  using S = lf::scope<Context>;
  co_await S::call(tok, fork_throw_and_cancel<Context>, tok);
  co_return;
}

#endif // LF_COMPILER_EXCEPTIONS

// ============================================================
//  Task for "call-based cancel: pre-stopped token, child skips its own work"
// ============================================================

template <typename Context>
auto call_pre_cancel_root(env<Context>, cancellation *tok, std::atomic<int> *ran) -> task<void, Context> {
  using S = lf::scope<Context>;
  // tok is already stopped before this call.
  // Root's cancel=nullptr → await_transform creates inner_with_cancel.
  // inner_with_cancel inherits tok → its own await_transform skips counting_task.
  co_await S::call(tok, inner_with_cancel<Context>, ran);
  co_return;
}

// ============================================================
//  Generic test runner
// ============================================================

template <typename Sch>
void run_cancel_tests(Sch &scheduler) {

  using Ctx = lf::context_t<Sch>;

  // ----------------------------------------------------------------
  // 1. No cancellation: normal execution still works
  // ----------------------------------------------------------------
  SECTION("no cancel: noop task completes") {
    auto recv = lf::schedule(scheduler, noop_task<Ctx>);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("no cancel: value task returns correct value") {
    auto recv = lf::schedule(scheduler, returning_task<Ctx>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == 99);
  }

  // ----------------------------------------------------------------
  // 2. Pre-cancelled token: child runs but its own sub-children are skipped
  //
  //    The cancel token is passed ONLY to the inner child (scope::call(&tok,
  //    inner_with_cancel)).  The root's cancel chain is nullptr, so the root's
  //    await_transform succeeds.  Inside inner_with_cancel, is_cancelled()
  //    returns true (its cancel=&tok, tok stopped), so counting_task is skipped.
  // ----------------------------------------------------------------
  SECTION("pre-cancel: grandchild is skipped") {
    cancellation tok;
    tok.request_stop();
    std::atomic<int> grandchild{0};

    auto recv = lf::schedule(scheduler, outer_with_cancel<Ctx>, &tok, &grandchild);
    REQUIRE(recv.valid());
    std::move(recv).get();
    REQUIRE(grandchild.load() == 0);
  }

  // ----------------------------------------------------------------
  // 3. Token signalled from within a FORKED child, then join (steals=0 path)
  //
  //    This exercises:
  //      (a) fork: child inline-completes before parent reaches join
  //      (b) join_awaitable::await_ready → false (steals=0, is_cancelled)
  //      (c) join_awaitable::await_suspend steals=0 special-case (bug fix)
  //      (d) handle_cancel → final_suspend cascade
  //
  //    post_join_ran must stay 0: code after co_await join() is unreachable.
  // ----------------------------------------------------------------
  SECTION("fork-cancel: post-join code unreachable after cancel (steals=0 path)") {
    cancellation tok;
    std::atomic<int> post_join_ran{0};

    auto recv = lf::schedule(scheduler, fork_outer<Ctx>, &tok, &post_join_ran);
    REQUIRE(recv.valid());
    std::move(recv).get();
    REQUIRE(post_join_ran.load() == 0);
  }

  // ----------------------------------------------------------------
  // 4. Second fork is skipped when token is already stopped at await_transform
  //
  //    fork_two_children: first fork signals tok, second fork is skipped
  //    because the parent's await_transform checks is_cancelled() → true.
  // ----------------------------------------------------------------
  SECTION("fork-cancel: second fork skipped when token already stopped") {
    cancellation tok;
    std::atomic<int> second_ran{0};

    auto recv = lf::schedule(scheduler, fork_two_outer<Ctx>, &tok, &second_ran);
    REQUIRE(recv.valid());
    std::move(recv).get();
    REQUIRE(second_ran.load() == 0);
  }

  // ----------------------------------------------------------------
  // 5. Return value is default-initialised when task is cancelled before
  //    co_return
  //
  //    inner_returning cascades at the join (via fork+signal); outer_returning
  //    writes the (never-set) return value, which stays 0.
  // ----------------------------------------------------------------
  SECTION("cancel before return: receiver holds default-initialised value") {
    cancellation tok;

    auto recv = lf::schedule(scheduler, outer_returning<Ctx>, &tok);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == 0);
  }

#if LF_COMPILER_EXCEPTIONS
  // ----------------------------------------------------------------
  // 6. Exception in forked child that also signals cancel
  //
  //    The child throws AND signals tok.  When the parent's join cascades via
  //    handle_cancel(), it discards the stashed exception (std::ignore =
  //    extract_exception).  The receiver must complete without throwing.
  // ----------------------------------------------------------------
  SECTION("cancel cleans up stashed exception: receiver does not throw") {
    cancellation tok;

    auto recv = lf::schedule(scheduler, fork_throw_outer<Ctx>, &tok);
    REQUIRE(recv.valid());
    REQUIRE_NOTHROW(std::move(recv).get());
  }
#endif // LF_COMPILER_EXCEPTIONS
}

} // namespace

// ============================================================
//  Token unit tests (no scheduler required)
// ============================================================

TEST_CASE("cancellation token: initial state is not stopped", "[cancel]") {
  cancellation tok;
  REQUIRE_FALSE(tok.stop_requested());
}

TEST_CASE("cancellation token: request_stop sets stopped", "[cancel]") {
  cancellation tok;
  tok.request_stop();
  REQUIRE(tok.stop_requested());
}

TEST_CASE("cancellation token: request_stop is idempotent", "[cancel]") {
  cancellation tok;
  tok.request_stop();
  tok.request_stop();
  REQUIRE(tok.stop_requested());
}

TEST_CASE("cancellation token: chain — neither stopped", "[cancel]") {
  cancellation parent;
  cancellation child{.parent = &parent};
  REQUIRE_FALSE(parent.stop_requested());
  REQUIRE_FALSE(child.stop_requested());
}

TEST_CASE("cancellation token: chain — stopping child does not affect parent", "[cancel]") {
  cancellation parent;
  cancellation child{.parent = &parent};
  child.request_stop();
  REQUIRE(child.stop_requested());
  REQUIRE_FALSE(parent.stop_requested()); // parent unaffected
}

TEST_CASE("cancellation token: chain — stopping parent does not affect child's own flag", "[cancel]") {
  cancellation parent;
  cancellation child{.parent = &parent};
  parent.request_stop();
  // stop_requested() only checks this node's flag:
  REQUIRE(parent.stop_requested());
  REQUIRE_FALSE(child.stop_requested());
  // But is_cancelled() (on a frame that holds a chain through child → parent)
  // would return true. That logic is tested indirectly via the scheduler tests.
}

// ============================================================
//  Schedule-based tests
// ============================================================

TEMPLATE_TEST_CASE("Inline cancellation", "[cancel]", mono_inline_ctx, poly_inline_ctx) {
  lf::inline_scheduler<TestType> scheduler;
  run_cancel_tests(scheduler);
}

TEMPLATE_TEST_CASE("Busy-pool cancellation", "[cancel]", mono_busy_pool, poly_busy_pool) {
  STATIC_REQUIRE(lf::scheduler<TestType>);
  for (std::size_t thr = 1; thr < 4; ++thr) {
    TestType pool{thr};
    run_cancel_tests(pool);
  }
}
