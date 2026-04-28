#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// ============================================================
// Awaitables and helpers
// ============================================================

namespace {

// ---- Pool type aliases (mirroring schedule.cpp:109-110) ----

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

// ---- Reference Fibonacci (non-coroutine) ----

[[nodiscard]]
auto fib_ref(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }
  return fib_ref(n - 1) + fib_ref(n - 2);
}

// ---- Plain hop_to: satisfies lf::awaitable<hop_to<Pool>, Pool::context_type> ----

template <typename Pool>
struct hop_to {
  Pool *target;
  std::atomic<std::thread::id> *resumed_on = nullptr;

  auto await_ready() noexcept -> bool { return false; }

  auto await_suspend(lf::sched_handle<typename Pool::context_type> h,
                     typename Pool::context_type & /*ctx*/) noexcept -> void {
    target->post(h);
  }

  auto await_resume() noexcept -> void {
    if (resumed_on != nullptr) {
      resumed_on->store(std::this_thread::get_id());
    }
  }
};

// ---- hop_to_throwing: await_suspend throws; used only with LF_COMPILER_EXCEPTIONS ----

template <typename Pool>
struct hop_to_throwing {
  Pool *target;

  auto await_ready() noexcept -> bool { return false; }

  // NOT noexcept — the throw path exercises prepare_release reversibility.
  auto await_suspend(lf::sched_handle<typename Pool::context_type> /*h*/,
                     typename Pool::context_type & /*ctx*/) -> void {
    LF_THROW(std::runtime_error{"hop"});
  }

  auto await_resume() noexcept -> void {}
};

// ---- hop_resume_throws: await_suspend is fine, await_resume throws ----

template <typename Pool>
struct hop_resume_throws {
  Pool *target;

  auto await_ready() noexcept -> bool { return false; }

  auto await_suspend(lf::sched_handle<typename Pool::context_type> h,
                     typename Pool::context_type & /*ctx*/) noexcept -> void {
    target->post(h);
  }

  auto await_resume() -> void { LF_THROW(std::runtime_error{"resume"}); }
};

// ---- Task functors used by tests below (must be at namespace scope: GCC ----
// rejects member templates inside function-local classes).

struct switch_inside_forked_child_wrapper {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other, std::int64_t *out) -> lf::task<void, Context>;
};

struct member_op_hop_task {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out) -> lf::task<void, Context>;
};

struct free_op_hop_task {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out) -> lf::task<void, Context>;
};

struct plain_hop_task {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other, std::atomic<bool> *flag) -> lf::task<void, Context>;
};

struct self_hop_task {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *p, std::atomic<bool> *flag) -> lf::task<void, Context>;
};

// Definitions of these task functors appear below, after all helper types
// (hop_member_op, hop_free_op, hop_child) are declared.

// ---- member-operator-co_await wrapper ----

template <typename Pool>
struct hop_member_op {
  Pool *target;
  auto operator co_await() noexcept -> hop_to<Pool> { return hop_to<Pool>{target}; }
};

// ---- free-operator-co_await wrapper ----

template <typename Pool>
struct hop_free_op {
  Pool *target;
};

template <typename Pool>
[[nodiscard]]
auto operator co_await(hop_free_op<Pool> h) noexcept -> hop_to<Pool> {
  return hop_to<Pool>{h.target};
}

// ============================================================
// Helper coroutines
// ============================================================

// Hop once to `other`, record resumed thread, return.
template <typename Context, typename Pool>
auto simple_hop_task(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out)
    -> lf::task<void, Context> {
  co_await hop_to<Pool>{other, out};
}

// Round-trip: A→B→A→B→A (4 hops from original pool A's perspective).
template <typename Context, typename Pool>
auto round_trip_task(lf::env<Context>, Pool *a, Pool *b, std::vector<std::thread::id> *ids, std::mutex *mu)
    -> lf::task<void, Context> {
  auto record = [&] {
    auto lock = std::scoped_lock(*mu);
    ids->push_back(std::this_thread::get_id());
  };
  record(); // point 0: on A
  co_await hop_to<Pool>{b};
  record(); // point 1: on B
  co_await hop_to<Pool>{a};
  record(); // point 2: back on A
  co_await hop_to<Pool>{b};
  record(); // point 3: on B
  co_await hop_to<Pool>{a};
  record(); // point 4: back on A
}

// Hop to `other`, then fork N children each computing fib(k), join, sum.
struct fib_child {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::int64_t n) -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }
    std::int64_t lhs = 0;
    std::int64_t rhs = 0;
    auto sc = co_await lf::scope();
    co_await sc.fork(&rhs, fib_child{}, n - 2);
    co_await sc.call(&lhs, fib_child{}, n - 1);
    co_await sc.join();
    co_return lhs + rhs;
  }
};

template <typename Context, typename Pool>
auto switch_then_fork_task(
    lf::env<Context>, Pool *other, int n_forks, std::int64_t k, std::int64_t *total_out)
    -> lf::task<void, Context> {
  co_await hop_to<Pool>{other};

  // After the hop, we are on a worker in `other`.
  std::vector<std::int64_t> results(static_cast<std::size_t>(n_forks), 0);
  auto sc = co_await lf::scope();
  for (int i = 0; i < n_forks; ++i) {
    co_await sc.fork(&results[static_cast<std::size_t>(i)], fib_child{}, k);
  }
  co_await sc.join();

  std::int64_t sum = 0;
  for (auto v : results) {
    sum += v;
  }
  *total_out = sum;
}

// Fork N children, then hop to `other`, then join.
// Exercises the "effectively stolen" path.
template <typename Context, typename Pool>
auto fork_then_switch_task(
    lf::env<Context>, Pool *other, int n_forks, std::int64_t k, std::int64_t *total_out)
    -> lf::task<void, Context> {
  std::vector<std::int64_t> results(static_cast<std::size_t>(n_forks), 0);
  auto sc = co_await lf::scope();
  for (int i = 0; i < n_forks; ++i) {
    co_await sc.fork(&results[static_cast<std::size_t>(i)], fib_child{}, k);
  }
  // Hand ourselves off to `other` while children may still be running on the
  // original pool.  This stresses resume_effectively_stolen.
  co_await hop_to<Pool>{other};
  co_await sc.join();

  std::int64_t sum = 0;
  for (auto v : results) {
    sum += v;
  }
  *total_out = sum;
}

// Recursive fib that switches pools at every even depth.
// Pool A is `pa`, pool B is `pb`; depth-parity selects target.
template <typename Context, typename Pool>
struct switch_fib_impl {
  static auto
  call(lf::env<Context>, std::int64_t n, Pool *pa, Pool *pb, int depth) -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }

    // Hop to the other pool at every odd depth.
    if (depth % 2 == 1) {
      Pool *other = (depth / 2) % 2 == 0 ? pb : pa;
      co_await hop_to<Pool>{other};
    }

    std::int64_t lhs = 0;
    std::int64_t rhs = 0;
    auto sc = co_await lf::scope();
    co_await sc.fork(&rhs, switch_fib_impl{}, n - 2, pa, pb, depth + 1);
    co_await sc.call(&lhs, switch_fib_impl{}, n - 1, pa, pb, depth + 1);
    co_await sc.join();
    co_return lhs + rhs;
  }

  template <typename... Args>
  auto operator()(lf::env<Context> e, Args &&...args) const -> lf::task<std::int64_t, Context> {
    return call(e, std::forward<Args>(args)...);
  }
};

// A forked child that hops to `other` before returning.
struct hop_child {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::int64_t *out, std::int64_t n) -> lf::task<void, Context> {
    co_await hop_to<Pool>{other};
    *out = fib_ref(n);
  }
};

// ---- Out-of-class definitions for the task functors declared earlier. ----

template <typename Context, typename Pool>
auto switch_inside_forked_child_wrapper::operator()(lf::env<Context>, Pool *other, std::int64_t *out)
    -> lf::task<void, Context> {
  auto sc = co_await lf::scope();
  co_await sc.fork(hop_child{}, other, out, std::int64_t{10});
  co_await sc.join();
}

template <typename Context, typename Pool>
auto member_op_hop_task::operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out)
    -> lf::task<void, Context> {
  co_await hop_member_op<Pool>{other};
  out->store(std::this_thread::get_id());
}

template <typename Context, typename Pool>
auto free_op_hop_task::operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out)
    -> lf::task<void, Context> {
  co_await hop_free_op<Pool>{other};
  out->store(std::this_thread::get_id());
}

template <typename Context, typename Pool>
auto plain_hop_task::operator()(lf::env<Context>, Pool *other, std::atomic<bool> *flag)
    -> lf::task<void, Context> {
  co_await hop_to<Pool>{other};
  flag->store(true);
}

template <typename Context, typename Pool>
auto self_hop_task::operator()(lf::env<Context>, Pool *p, std::atomic<bool> *flag)
    -> lf::task<void, Context> {
  co_await hop_to<Pool>{p};
  flag->store(true);
}

// Compute a value, hop, then return it.
template <typename Context, typename Pool>
auto value_across_switch(lf::env<Context>, Pool *other, std::int64_t *out) -> lf::task<void, Context> {
  std::int64_t x = 41;
  co_await hop_to<Pool>{other};
  *out = x + 1;
}

// K hops between a and b (alternating), increments counter each time it resumes.
template <typename Context, typename Pool>
auto multi_hop_task(lf::env<Context>, Pool *a, Pool *b, int k, std::atomic<int> *completed)
    -> lf::task<void, Context> {
  for (int i = 0; i < k; ++i) {
    Pool *dest = (i % 2 == 0) ? b : a;
    co_await hop_to<Pool>{dest};
  }
  completed->fetch_add(1, std::memory_order_relaxed);
}

// Binary-tree recursion: fork two children at each node; at odd levels, hop to other pool first.
struct hop_tree {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, int depth, Pool *a, Pool *b, std::atomic<int> *leaf_count)
      -> lf::task<void, Context> {
    if (depth == 0) {
      leaf_count->fetch_add(1, std::memory_order_relaxed);
      co_return;
    }

    if (depth % 2 == 1) {
      // Hop to the other pool before forking; forces the "effectively stolen"
      // paths because children will end up running on the opposite pool.
      Pool *other = b; // always hop toward b on odd levels; doesn't matter which
      (void)a;
      co_await hop_to<Pool>{other};
    }

    auto sc = co_await lf::scope();
    co_await sc.fork(hop_tree{}, depth - 1, a, b, leaf_count);
    co_await sc.call(hop_tree{}, depth - 1, a, b, leaf_count);
    co_await sc.join();
  }
};

#if LF_COMPILER_EXCEPTIONS

// Root task that performs a throwing hop.
template <typename Context, typename Pool>
auto throwing_hop_root(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
  co_await hop_to_throwing<Pool>{other};
}

// Child that performs a throwing hop — exception must propagate to parent's join.
struct throwing_hop_child {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
    co_await hop_to_throwing<Pool>{other};
  }
};

template <typename Context, typename Pool>
auto fork_throwing_hop(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
  auto sc = co_await lf::scope();
  co_await sc.fork_drop(throwing_hop_child{}, other);
  co_await sc.join();
}

// Root task that performs a resume-throwing hop.
template <typename Context, typename Pool>
auto resume_throw_root(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
  co_await hop_resume_throws<Pool>{other};
}

#endif // LF_COMPILER_EXCEPTIONS

// ============================================================
// Concept conformance helpers (compile-time only)
// ============================================================

namespace concept_checks {

// Use the test_context from concepts.cpp's pattern.
using test_stack = lf::geometric_stack<>;
using test_context = lf::mono_context<test_stack, lf::adapt_vector<>>;

// A well-formed awaitable for test_context.
struct good_awaitable {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<test_context>, test_context &) noexcept -> void {}
  auto await_resume() noexcept -> void {}
};

// await_suspend takes int& instead of the right context type.
struct bad_context_type {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<test_context>, int &) -> void {}
  auto await_resume() -> void {}
};

// await_ready returns int, not convertible to bool... actually int IS convertible;
// let's use a struct that is NOT convertible to bool.
struct non_bool_ready {
  struct not_bool {};
  auto await_ready() noexcept -> not_bool { return {}; }
  auto await_suspend(lf::sched_handle<test_context>, test_context &) noexcept -> void {}
  auto await_resume() noexcept -> void {}
};

// await_suspend returns non-void.
struct non_void_suspend {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<test_context>, test_context &) noexcept -> int { return 0; }
  auto await_resume() noexcept -> void {}
};

// Good awaitable but NOT noexcept on await_suspend.
struct throwing_suspend_awaitable {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<test_context>, test_context &) -> void {}
  auto await_resume() noexcept -> void {}
};

} // namespace concept_checks

} // namespace

// ============================================================
// Test cases
// ============================================================

// ---- 1. One-shot switch ----

TEMPLATE_TEST_CASE("explicit-sched: one-shot switch", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::atomic<std::thread::id> resumed_on;
  std::thread::id const caller_id = std::this_thread::get_id();

  auto recv = lf::schedule(pool_a, simple_hop_task<lf::context_t<TestType>, TestType>, &pool_b, &resumed_on);
  std::move(recv).get();

  REQUIRE(resumed_on.load() != std::thread::id{});
  // The task was resumed on a worker thread, not the test thread.
  REQUIRE(resumed_on.load() != caller_id);
}

// ---- 2. Round-trip A→B→A→B→A ----

TEMPLATE_TEST_CASE("explicit-sched: round-trip", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::vector<std::thread::id> ids;
  std::mutex mu;

  auto recv =
      lf::schedule(pool_a, round_trip_task<lf::context_t<TestType>, TestType>, &pool_a, &pool_b, &ids, &mu);
  std::move(recv).get();

  REQUIRE(ids.size() == 5);
  // Every recorded id must be a worker thread, not the test thread.
  std::thread::id const caller_id = std::this_thread::get_id();
  for (auto const &id : ids) {
    REQUIRE(id != caller_id);
    REQUIRE(id != std::thread::id{});
  }
}

// ---- 3. Switch then fork-join ----

TEMPLATE_TEST_CASE("explicit-sched: switch then fork-join", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  constexpr int n_forks = 8;
  constexpr std::int64_t k = 10;
  std::int64_t total = 0;

  auto recv = lf::schedule(
      pool_a, switch_then_fork_task<lf::context_t<TestType>, TestType>, &pool_b, n_forks, k, &total);
  std::move(recv).get();

  REQUIRE(total == fib_ref(k) * n_forks);
}

// ---- 4. Fork then switch then join ----

TEMPLATE_TEST_CASE("explicit-sched: fork then switch then join", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  constexpr std::int64_t k = 8;

  for (int n : {1, 8, 64}) {
    DYNAMIC_SECTION("n_forks=" << n) {
      std::int64_t total = 0;
      auto recv = lf::schedule(
          pool_a, fork_then_switch_task<lf::context_t<TestType>, TestType>, &pool_b, n, k, &total);
      std::move(recv).get();
      REQUIRE(total == fib_ref(k) * n);
    }
  }
}

// ---- 5. Nested fork/switch/recursive-fib ----

TEMPLATE_TEST_CASE("explicit-sched: nested fork/switch/join", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  for (std::int64_t n : {12, 16, 20}) {
    DYNAMIC_SECTION("n=" << n) {
      using SwitchFib = switch_fib_impl<lf::context_t<TestType>, TestType>;
      auto recv = lf::schedule(pool_a, SwitchFib{}, n, &pool_a, &pool_b, 0);
      std::int64_t result = std::move(recv).get();
      REQUIRE(result == fib_ref(n));
    }
  }
}

// ---- 6. Switch inside forked child ----

TEMPLATE_TEST_CASE("explicit-sched: switch inside forked child", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::int64_t child_result = 0;

  auto recv = lf::schedule(pool_a, switch_inside_forked_child_wrapper{}, &pool_b, &child_result);

  std::move(recv).get();
  REQUIRE(child_result == fib_ref(10));
}

// ---- 7. Many independent tasks ----

TEMPLATE_TEST_CASE("explicit-sched: many independent tasks", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  constexpr int M = 256;
  constexpr int K = 8;
  std::atomic<int> completed{0};

  // Schedule all tasks upfront, then drain.
  std::vector<lf::receiver<void>> receivers;
  receivers.reserve(M);
  for (int i = 0; i < M; ++i) {
    receivers.push_back(lf::schedule(
        pool_a, multi_hop_task<lf::context_t<TestType>, TestType>, &pool_a, &pool_b, K, &completed));
  }
  for (auto &r : receivers) {
    std::move(r).get();
  }

  REQUIRE(completed.load() == M);
}

// ---- 8. Returns non-void value across switch ----

TEMPLATE_TEST_CASE("explicit-sched: returns non-void value across switch",
                   "[explicit-sched]",
                   mono_pool,
                   poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::int64_t out = 0;
  auto recv = lf::schedule(pool_a, value_across_switch<lf::context_t<TestType>, TestType>, &pool_b, &out);
  std::move(recv).get();
  REQUIRE(out == 42);
}

// ---- 9. Member operator co_await ----

TEMPLATE_TEST_CASE("explicit-sched: member operator co_await", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::thread::id const caller_id = std::this_thread::get_id();
  std::atomic<std::thread::id> resumed_on;

  auto recv = lf::schedule(pool_a, member_op_hop_task{}, &pool_b, &resumed_on);
  std::move(recv).get();

  REQUIRE(resumed_on.load() != std::thread::id{});
  REQUIRE(resumed_on.load() != caller_id);
}

// ---- 10. Free operator co_await ----

TEMPLATE_TEST_CASE("explicit-sched: free operator co_await", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::thread::id const caller_id = std::this_thread::get_id();
  std::atomic<std::thread::id> resumed_on;

  auto recv = lf::schedule(pool_a, free_op_hop_task{}, &pool_b, &resumed_on);
  std::move(recv).get();

  REQUIRE(resumed_on.load() != std::thread::id{});
  REQUIRE(resumed_on.load() != caller_id);
}

// ---- 11. Plain awaitable (no operator co_await) ----
// hop_to is already a plain awaitable; this test focuses on it explicitly.

TEMPLATE_TEST_CASE("explicit-sched: plain awaitable", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  std::atomic<bool> ran{false};

  auto recv = lf::schedule(pool_a, plain_hop_task{}, &pool_b, &ran);
  std::move(recv).get();
  REQUIRE(ran.load());
}

// ---- 12. Switch to same pool ----

TEMPLATE_TEST_CASE("explicit-sched: switch to same pool", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool{2};

  std::atomic<bool> ran{false};

  auto recv = lf::schedule(pool, self_hop_task{}, &pool, &ran);
  std::move(recv).get();
  REQUIRE(ran.load());
}

// ---- 13/14. Exception tests ----

#if LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE("explicit-sched: throwing await_suspend", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  SECTION("at root") {
    auto recv = lf::schedule(pool_a, throwing_hop_root<lf::context_t<TestType>, TestType>, &pool_b);
    REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
  }

  SECTION("inside forked child") {
    auto recv = lf::schedule(pool_a, fork_throwing_hop<lf::context_t<TestType>, TestType>, &pool_b);
    REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
  }
}

TEMPLATE_TEST_CASE("explicit-sched: throwing await_resume", "[explicit-sched]", mono_pool, poly_pool) {
  TestType pool_a{2};
  TestType pool_b{2};

  auto recv = lf::schedule(pool_a, resume_throw_root<lf::context_t<TestType>, TestType>, &pool_b);
  REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
}

#endif // LF_COMPILER_EXCEPTIONS

// ---- 15. Concept conformance (compile-time) ----

TEST_CASE("explicit-sched: concept conformance", "[explicit-sched]") {
  using namespace concept_checks;

  // A well-formed awaitable satisfies lf::awaitable.
  STATIC_REQUIRE(lf::awaitable<good_awaitable, test_context>);

  // Wrong context type in await_suspend.
  STATIC_REQUIRE_FALSE(lf::awaitable<bad_context_type, test_context>);

  // await_ready returns a type not convertible to bool.
  STATIC_REQUIRE_FALSE(lf::awaitable<non_bool_ready, test_context>);

  // await_suspend returns non-void.
  STATIC_REQUIRE_FALSE(lf::awaitable<non_void_suspend, test_context>);

  // throwing_suspend_awaitable still satisfies awaitable (noexcept on suspend
  // is not a requirement of the awaitable concept itself).
  STATIC_REQUIRE(lf::awaitable<throwing_suspend_awaitable, test_context>);
}

// ---- 16. Stress: hop binary tree ----

TEMPLATE_TEST_CASE("explicit-sched: stress hop-binary-tree",
                   "[explicit-sched][stress]",
                   mono_pool,
                   poly_pool) {
  constexpr int depth = 12;
  constexpr int expect_leaves = 1 << depth;

  std::size_t const hw = static_cast<std::size_t>(std::thread::hardware_concurrency());

  for (std::size_t thr : {std::size_t{2}, std::size_t{4}, std::size_t{8}}) {
    if (thr > hw) {
      break;
    }
    DYNAMIC_SECTION("threads=" << thr) {
      std::atomic<int> leaf_count{0};
      // Split threads evenly between two pools.
      std::size_t const half = thr / 2 + (thr % 2);
      TestType pool_a{half};
      TestType pool_b{thr - half + 1}; // at least 1

      auto recv = lf::schedule(pool_a, hop_tree{}, depth, &pool_a, &pool_b, &leaf_count);
      std::move(recv).get();
      REQUIRE(leaf_count.load() == expect_leaves);
    }
  }
}
