#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// ============================================================
// Tests for the explicit scheduling machinery (see
// docs/explicit-scheduling.md). The goal is to exercise:
//
//   1. switch_awaitable's stack-handoff & "effectively stolen"
//      drain logic across all interesting suspension shapes
//      (root, child, before/after fork, recursion).
//   2. Concept conformance for `lf::awaitable`.
//   3. Exception propagation from both `await_suspend` (before
//      publishing) and `await_resume` (after the hop).
//
// Where possible, tests verify *which pool* the task resumed on
// (not just "not the caller thread"). To do so we precompute the
// worker-thread-id set of each pool by submitting a barrier-blocked
// task per worker and capturing its TID.
// ============================================================

namespace {

// ---- Pool aliases ---------------------------------------------------------

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

constexpr std::array<std::size_t, 3> k_worker_counts{1, 2, 4};

// ---- Reference fib --------------------------------------------------------

[[nodiscard]]
auto fib_ref(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }
  return fib_ref(n - 1) + fib_ref(n - 2);
}

// ---- Worker TID discovery -------------------------------------------------
//
// To force every worker in `pool` to expose its TID, post N tasks (one per
// worker) that each block on a shared barrier. Because each task blocks
// before completing, every worker must dequeue exactly one task — once all
// are dequeued and the barrier opens, each task records its TID and
// returns. The result set therefore contains exactly N distinct TIDs.

struct record_tid_fn {
  template <typename Context>
  static auto
  operator()(lf::env<Context>, std::barrier<> *sync, std::thread::id *out) -> lf::task<void, Context> {
    *out = std::this_thread::get_id();
    sync->arrive_and_wait();
    co_return;
  }
};

template <typename Pool>
[[nodiscard]]
auto discover_worker_tids(Pool &pool, std::size_t n_workers) -> std::unordered_set<std::thread::id> {
  std::barrier sync(static_cast<std::ptrdiff_t>(n_workers));
  std::vector<std::thread::id> tids(n_workers);
  std::vector<lf::receiver<void>> recvs;
  recvs.reserve(n_workers);
  for (std::size_t i = 0; i < n_workers; ++i) {
    recvs.push_back(lf::schedule(pool, record_tid_fn{}, &sync, &tids[i]));
  }
  for (auto &r : recvs) {
    std::move(r).get();
  }
  std::unordered_set<std::thread::id> set(tids.begin(), tids.end());
  REQUIRE(set.size() == n_workers); // Sanity: each worker ran exactly one barrier task.
  return set;
}

// ============================================================
// Awaitables
// ============================================================

// Plain awaitable: hops to `target` and optionally records the TID it
// resumed on.
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
      resumed_on->store(std::this_thread::get_id(), std::memory_order_relaxed);
    }
  }
};

// await_suspend throws *before* publishing the handle. Exercises
// `prepare_release` reversibility — the parent must be resumed inline
// with the exception live.
template <typename Pool>
struct hop_throw_in_suspend {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<typename Pool::context_type>, typename Pool::context_type &) -> void {
    LF_THROW(std::runtime_error{"suspend"});
  }
  auto await_resume() noexcept -> void {}
};

// Successfully hops, then throws from `await_resume` on the destination
// thread. Exception must propagate through the rest of the task.
template <typename Pool>
struct hop_throw_in_resume {
  Pool *target;
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<typename Pool::context_type> h, typename Pool::context_type &) noexcept
      -> void {
    target->post(h);
  }
  auto await_resume() -> void { LF_THROW(std::runtime_error{"resume"}); }
};

// Posts the handle from a *separate* detached thread, simulating an
// I/O-style awaitable where the source worker can return before the
// handle is published.
template <typename Pool>
struct hop_deferred_post {
  Pool *target;

  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<typename Pool::context_type> h, typename Pool::context_type &) noexcept
      -> void {
    std::thread([h, t = target]() mutable {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      t->post(h);
    }).detach();
  }
  auto await_resume() noexcept -> void {}
};

// Member operator co_await wrapper.
template <typename Pool>
struct hop_member_op {
  Pool *target;
  auto operator co_await() noexcept -> hop_to<Pool> { return hop_to<Pool>{target}; }
};

// Free operator co_await wrapper.
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
// Task functors (all members templated on Context — no out-of-class
// member-template definitions are needed because each functor only
// references awaitables / functors declared above it).
// ============================================================

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

// Hop once and record the resumed TID.
struct hop_record_tid {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out) -> lf::task<void, Context> {
    co_await hop_to<Pool>{other, out};
  }
};

// A→B→A→B→A; record the TID at each of the 5 points.
struct round_trip {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *a, Pool *b, std::array<std::thread::id, 5> *ids)
      -> lf::task<void, Context> {
    (*ids)[0] = std::this_thread::get_id();
    co_await hop_to<Pool>{b};
    (*ids)[1] = std::this_thread::get_id();
    co_await hop_to<Pool>{a};
    (*ids)[2] = std::this_thread::get_id();
    co_await hop_to<Pool>{b};
    (*ids)[3] = std::this_thread::get_id();
    co_await hop_to<Pool>{a};
    (*ids)[4] = std::this_thread::get_id();
  }
};

// Hop to `other`, then fork `n_forks` fib(k) computations and sum.
struct hop_then_fib_sum {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, int n_forks, std::int64_t k) -> lf::task<std::int64_t, Context> {
    co_await hop_to<Pool>{other};
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
    co_return sum;
  }
};

// Fork first, *then* hop, then join. Stresses
// `resume_effectively_stolen` — at suspension the parent has live
// children on the original pool's WSQ.
struct fib_then_hop_sum {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, int n_forks, std::int64_t k) -> lf::task<std::int64_t, Context> {
    std::vector<std::int64_t> results(static_cast<std::size_t>(n_forks), 0);
    auto sc = co_await lf::scope();
    for (int i = 0; i < n_forks; ++i) {
      co_await sc.fork(&results[static_cast<std::size_t>(i)], fib_child{}, k);
    }
    co_await hop_to<Pool>{other};
    co_await sc.join();
    std::int64_t sum = 0;
    for (auto v : results) {
      sum += v;
    }
    co_return sum;
  }
};

// Recursive fib that hops at every odd depth, alternating destinations.
struct switch_fib {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, std::int64_t n, Pool *pa, Pool *pb, int depth)
      -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }
    if (depth % 2 == 1) {
      Pool *dest = (depth / 2) % 2 == 0 ? pb : pa;
      co_await hop_to<Pool>{dest};
    }
    std::int64_t lhs = 0;
    std::int64_t rhs = 0;
    auto sc = co_await lf::scope();
    co_await sc.fork(&rhs, switch_fib{}, n - 2, pa, pb, depth + 1);
    co_await sc.call(&lhs, switch_fib{}, n - 1, pa, pb, depth + 1);
    co_await sc.join();
    co_return lhs + rhs;
  }
};

// A forked child that hops to `other` before computing fib(n).
struct hop_fib_child {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other, std::int64_t n) -> lf::task<std::int64_t, Context> {
    co_await hop_to<Pool>{other};
    co_return fib_ref(n);
  }
};

struct fork_hop_fib {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other, std::int64_t n) -> lf::task<std::int64_t, Context> {
    std::int64_t result = 0;
    auto sc = co_await lf::scope();
    co_await sc.fork(&result, hop_fib_child{}, other, n);
    co_await sc.join();
    co_return result;
  }
};

// Alternate hops between a/b for `k` iterations; each hop records the
// resumed TID into `ids` (held under `mu`).
struct alternating_hops {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *a, Pool *b, int k, std::vector<std::thread::id> *ids, std::mutex *mu)
      -> lf::task<void, Context> {
    for (int i = 0; i < k; ++i) {
      Pool *dest = (i % 2 == 0) ? b : a;
      co_await hop_to<Pool>{dest};
      auto lock = std::scoped_lock(*mu);
      ids->push_back(std::this_thread::get_id());
    }
  }
};

// Hop to one's own pool — tests the self-hop / single-worker path.
struct self_hop {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *p, std::atomic<std::thread::id> *out) -> lf::task<void, Context> {
    co_await hop_to<Pool>{p, out};
  }
};

struct member_op_hop {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out) -> lf::task<void, Context> {
    co_await hop_member_op<Pool>{other};
    out->store(std::this_thread::get_id(), std::memory_order_relaxed);
  }
};

struct free_op_hop {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out) -> lf::task<void, Context> {
    co_await hop_free_op<Pool>{other};
    out->store(std::this_thread::get_id(), std::memory_order_relaxed);
  }
};

struct deferred_hop_task {
  template <typename Context, typename Pool>
  static auto
  operator()(lf::env<Context>, Pool *other, std::atomic<std::thread::id> *out) -> lf::task<void, Context> {
    co_await hop_deferred_post<Pool>{other};
    out->store(std::this_thread::get_id(), std::memory_order_relaxed);
  }
};

// Binary-tree recursion that hops at odd levels.
struct hop_tree {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, int depth, Pool *a, Pool *b, std::atomic<int> *leaf_count)
      -> lf::task<void, Context> {
    if (depth == 0) {
      leaf_count->fetch_add(1, std::memory_order_relaxed);
      co_return;
    }
    if (depth % 2 == 1) {
      (void)a;
      co_await hop_to<Pool>{b};
    }
    auto sc = co_await lf::scope();
    co_await sc.fork(hop_tree{}, depth - 1, a, b, leaf_count);
    co_await sc.call(hop_tree{}, depth - 1, a, b, leaf_count);
    co_await sc.join();
  }
};

#if LF_COMPILER_EXCEPTIONS

struct throw_in_suspend_root {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
    (void)other;
    co_await hop_throw_in_suspend<Pool>{};
  }
};

struct throw_in_suspend_child {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
    (void)other;
    co_await hop_throw_in_suspend<Pool>{};
  }
};

struct fork_throw_in_suspend {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
    auto sc = co_await lf::scope();
    co_await sc.fork_drop(throw_in_suspend_child{}, other);
    co_await sc.join();
  }
};

struct throw_in_resume_root {
  template <typename Context, typename Pool>
  static auto operator()(lf::env<Context>, Pool *other) -> lf::task<void, Context> {
    co_await hop_throw_in_resume<Pool>{other};
  }
};

#endif // LF_COMPILER_EXCEPTIONS

// ============================================================
// Concept conformance helpers (compile-time only)
//
// These are scoped to "extra" cases that aren't exercised in
// concepts.cpp — namely the `await_suspend` return-type and
// noexcept-vs-throwing distinctions specific to switch_awaitable.
// ============================================================

namespace concept_checks {

using test_stack = lf::geometric_stack<>;
using test_context = lf::mono_context<test_stack, lf::adapt_vector<>>;

// await_ready returns a type that is *not* convertible to bool.
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

// noexcept on await_suspend is *not* a requirement of `lf::awaitable`
// (only of `nothrow_await_suspend`).
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

// ---- 1. One-shot switch ---------------------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: one-shot", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      auto a_tids = discover_worker_tids(pool_a, n);
      auto b_tids = discover_worker_tids(pool_b, n);

      std::atomic<std::thread::id> resumed_on;
      auto recv = lf::schedule(pool_a, hop_record_tid{}, &pool_b, &resumed_on);
      std::move(recv).get();

      auto tid = resumed_on.load();
      REQUIRE(b_tids.contains(tid));
      REQUIRE_FALSE(a_tids.contains(tid));
    }
  }
}

// ---- 2. Round-trip A→B→A→B→A ---------------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: round-trip", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      auto a_tids = discover_worker_tids(pool_a, n);
      auto b_tids = discover_worker_tids(pool_b, n);

      std::array<std::thread::id, 5> ids{};
      auto recv = lf::schedule(pool_a, round_trip{}, &pool_a, &pool_b, &ids);
      std::move(recv).get();

      // Points 0, 2, 4 are on A; 1, 3 are on B.
      for (std::size_t i : {std::size_t{0}, std::size_t{2}, std::size_t{4}}) {
        REQUIRE(a_tids.contains(ids[i]));
      }
      for (std::size_t i : {std::size_t{1}, std::size_t{3}}) {
        REQUIRE(b_tids.contains(ids[i]));
      }
    }
  }
}

// ---- 3. Switch then fork-join --------------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: switch then fork-join", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      constexpr int n_forks = 8;
      constexpr std::int64_t k = 10;

      auto recv = lf::schedule(pool_a, hop_then_fib_sum{}, &pool_b, n_forks, k);
      auto sum = std::move(recv).get();
      REQUIRE(sum == fib_ref(k) * n_forks);
    }
  }
}

// ---- 4. Fork then switch then join ---------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: fork then switch then join", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool_a{n_workers};
      TestType pool_b{n_workers};

      constexpr std::int64_t k = 8;

      for (int n : {1, 8, 64}) {
        DYNAMIC_SECTION("n_forks=" << n) {
          auto recv = lf::schedule(pool_a, fib_then_hop_sum{}, &pool_b, n, k);
          auto sum = std::move(recv).get();
          REQUIRE(sum == fib_ref(k) * n);
        }
      }
    }
  }
}

// ---- 5. Nested fork/switch/recursive-fib ---------------------------------

TEMPLATE_TEST_CASE("explicit-sched: nested fork/switch/join", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool_a{n_workers};
      TestType pool_b{n_workers};

      for (std::int64_t n : {12, 16, 20}) {
        DYNAMIC_SECTION("n=" << n) {
          auto recv = lf::schedule(pool_a, switch_fib{}, n, &pool_a, &pool_b, 0);
          auto result = std::move(recv).get();
          REQUIRE(result == fib_ref(n));
        }
      }
    }
  }
}

// ---- 6. Switch inside forked child ---------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: switch inside forked child", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      constexpr std::int64_t k = 10;
      auto recv = lf::schedule(pool_a, fork_hop_fib{}, &pool_b, k);
      auto result = std::move(recv).get();
      REQUIRE(result == fib_ref(k));
    }
  }
}

// ---- 7. Many independent multi-hop tasks (stress) ------------------------

TEMPLATE_TEST_CASE("explicit-sched: independent multi-hop tasks", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      auto a_tids = discover_worker_tids(pool_a, n);
      auto b_tids = discover_worker_tids(pool_b, n);

      constexpr int M = 64;
      constexpr int K = 8;

      std::vector<std::vector<std::thread::id>> per_task_ids(M);
      std::vector<std::mutex> per_task_mu(M);
      std::vector<lf::receiver<void>> recvs;
      recvs.reserve(M);
      for (std::size_t i = 0; i < M; ++i) {
        recvs.push_back(
            lf::schedule(pool_a, alternating_hops{}, &pool_a, &pool_b, K, &per_task_ids[i], &per_task_mu[i]));
      }
      for (auto &r : recvs) {
        std::move(r).get();
      }

      // Each hop must land on the addressed pool.
      for (auto const &ids : per_task_ids) {
        REQUIRE(ids.size() == K);
        for (int i = 0; i < K; ++i) {
          // i=0 → b, i=1 → a, ...
          auto const &expect = (i % 2 == 0) ? b_tids : a_tids;
          REQUIRE(expect.contains(ids[static_cast<std::size_t>(i)]));
        }
      }
    }
  }
}

// ---- 8. Member operator co_await -----------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: member operator co_await", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};
      auto b_tids = discover_worker_tids(pool_b, n);

      std::atomic<std::thread::id> resumed_on;
      auto recv = lf::schedule(pool_a, member_op_hop{}, &pool_b, &resumed_on);
      std::move(recv).get();
      REQUIRE(b_tids.contains(resumed_on.load()));
    }
  }
}

// ---- 9. Free operator co_await -------------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: free operator co_await", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};
      auto b_tids = discover_worker_tids(pool_b, n);

      std::atomic<std::thread::id> resumed_on;
      auto recv = lf::schedule(pool_a, free_op_hop{}, &pool_b, &resumed_on);
      std::move(recv).get();
      REQUIRE(b_tids.contains(resumed_on.load()));
    }
  }
}

// ---- 10. Deferred post (handle published from a foreign thread) -----------

// Simulates an I/O-style awaitable: `await_suspend` returns immediately
// without publishing the handle, then a separate thread eventually
// posts it. Verifies that the source worker can leave switch_awaitable
// (and possibly drain effectively-stolen tasks) before the destination
// resumes.
TEMPLATE_TEST_CASE("explicit-sched: posted from foreign thread", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};
      auto b_tids = discover_worker_tids(pool_b, n);

      std::atomic<std::thread::id> resumed_on;
      auto recv = lf::schedule(pool_a, deferred_hop_task{}, &pool_b, &resumed_on);
      std::move(recv).get();
      REQUIRE(b_tids.contains(resumed_on.load()));
    }
  }
}

// ---- 11. Self-hop --------------------------------------------------------

// A hop into one's own pool must resume on a worker of that pool.
// With 1 worker, the resumed TID must be the same worker.
TEMPLATE_TEST_CASE("explicit-sched: self-hop", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool{n};
      auto tids = discover_worker_tids(pool, n);

      std::atomic<std::thread::id> resumed_on;
      auto recv = lf::schedule(pool, self_hop{}, &pool, &resumed_on);
      std::move(recv).get();
      REQUIRE(tids.contains(resumed_on.load()));
    }
  }
}

// ---- 12. Exception in await_suspend --------------------------------------

#if LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE("explicit-sched: throwing await_suspend", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      SECTION("at root") {
        auto recv = lf::schedule(pool_a, throw_in_suspend_root{}, &pool_b);
        REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
      }

      SECTION("inside forked child") {
        auto recv = lf::schedule(pool_a, fork_throw_in_suspend{}, &pool_b);
        REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
      }
    }
  }
}

// ---- 13. Exception in await_resume ---------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: throwing await_resume", "[explicit-sched]", mono_pool, poly_pool) {
  for (auto n : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n) {
      TestType pool_a{n};
      TestType pool_b{n};

      auto recv = lf::schedule(pool_a, throw_in_resume_root{}, &pool_b);
      REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
    }
  }
}

#endif // LF_COMPILER_EXCEPTIONS

// ---- 14. Concept conformance (compile-time) ------------------------------

// concepts.cpp covers the basic positive case + the wrong-context-type
// negative case. The cases below cover return-type and noexcept-vs-throwing
// requirements specific to switch_awaitable.
TEST_CASE("explicit-sched: concept conformance", "[explicit-sched]") {
  using namespace concept_checks;

  STATIC_REQUIRE_FALSE(lf::awaitable<non_bool_ready, test_context>);
  STATIC_REQUIRE_FALSE(lf::awaitable<non_void_suspend, test_context>);

  // Throwing await_suspend still satisfies awaitable; only nothrow_await_suspend
  // (used as a noexcept-spec on switch_awaitable::await_suspend) is finer.
  STATIC_REQUIRE(lf::awaitable<throwing_suspend_awaitable, test_context>);
}

// ---- 15. Stress: hop binary tree -----------------------------------------

TEMPLATE_TEST_CASE("explicit-sched: hop-binary-tree", "[explicit-sched][stress]", mono_pool, poly_pool) {
  constexpr int depth = 12;
  constexpr int expect_leaves = 1 << depth;

  std::size_t const hw = static_cast<std::size_t>(std::thread::hardware_concurrency());

  for (std::size_t thr : {std::size_t{2}, std::size_t{4}, std::size_t{8}}) {
    if (thr > hw) {
      break;
    }
    DYNAMIC_SECTION("threads=" << thr) {
      std::atomic<int> leaf_count{0};
      std::size_t const half = thr / 2 + (thr % 2);
      TestType pool_a{half};
      TestType pool_b{thr - half + 1}; // at least 1

      auto recv = lf::schedule(pool_a, hop_tree{}, depth, &pool_a, &pool_b, &leaf_count);
      std::move(recv).get();
      REQUIRE(leaf_count.load() == expect_leaves);
    }
  }
}
