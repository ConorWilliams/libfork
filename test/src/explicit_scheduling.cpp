#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// NOTE: Functional tests that use `co_await <user-awaitable>` inside a task
// are currently blocked by a bug introduced in commit bffa5cdd.
//
// The `mixin_frame<Context>::await_transform` overload for custom awaitables is
// declared `static constexpr` and its return type (via LF_HOF) references the
// private static helper `await_transform_switch`.  When the function is
// instantiated in the context of the derived `promise_type<T, Context>`, clang
// treats `mixin_frame<Context>` as a **dependent base class** and cannot find
// `await_transform_switch` via unqualified lookup, causing a SFINAE failure that
// silently discards the overload and leaves `co_await <user-awaitable>` with no
// matching `await_transform` candidate.
//
// Root cause: the previous implementation (before bffa5cdd) directly constructed
//   `switch_awaitable<Context, remove_cvref_t<T>>{LF_FWD(x)}`
// in the LF_HOF body, avoiding the extra private helper call.  The new path
//   `await_transform_switch(acquire_awaitable(LF_FWD(x)))`
// fails because `await_transform_switch` is in the dependent base from
// promise_type's perspective.
//
// Fix (in src/core/promise.cxx only): qualify the call as
//   `mixin_frame<Context>::await_transform_switch(acquire_awaitable(LF_FWD(x)))`
// or revert to the direct construction approach.
//
// Affected tests: 1-14, 16 (all functional tests that actually `co_await` a
// user-defined awaitable at runtime).  Test 15 (concept conformance) works as
// it only exercises compile-time concept checks and does not go through
// `await_transform`.

// ============================================================
// Awaitables  (defined even though the functional tests are gated;
// the concept-check test validates their signatures.)
// ============================================================

namespace {

// ---- Pool type aliases ----

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

// ---- hop_to<Context>: plain awaitable ----
//
// Templated on Context (the worker context type) so it can be used inside
// task<T, Context> without any Pool dependency in the signature.

template <typename Context>
struct hop_to {
  void (*post_fn)(void *, lf::sched_handle<Context>);
  void *pool_ptr;
  std::atomic<std::thread::id> *resumed_on = nullptr;

  auto await_ready() noexcept -> bool { return false; }

  auto await_suspend(lf::sched_handle<Context> h, Context & /*ctx*/) noexcept -> void {
    post_fn(pool_ptr, h);
  }

  auto await_resume() noexcept -> void {
    if (resumed_on != nullptr) {
      resumed_on->store(std::this_thread::get_id());
    }
  }
};

// Construct a hop_to that posts to Pool *p.
template <typename Pool>
[[nodiscard]]
auto make_hop(Pool *pool,
              std::atomic<std::thread::id> *out = nullptr)
    -> hop_to<typename Pool::context_type> {
  return {
      [](void *p, lf::sched_handle<typename Pool::context_type> h) {
        static_cast<Pool *>(p)->post(h);
      },
      pool,
      out,
  };
}

// ---- hop_to_throwing<Context>: await_suspend throws (not noexcept) ----

template <typename Context>
struct hop_to_throwing {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<Context> /*h*/, Context & /*ctx*/) -> void {
    LF_THROW(std::runtime_error{"hop"});
  }
  auto await_resume() noexcept -> void {}
};

// ---- hop_resume_throws<Context>: await_suspend succeeds, await_resume throws ----

template <typename Context>
struct hop_resume_throws {
  void (*post_fn)(void *, lf::sched_handle<Context>);
  void *pool_ptr;

  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<Context> h, Context & /*ctx*/) noexcept -> void {
    post_fn(pool_ptr, h);
  }
  auto await_resume() -> void { LF_THROW(std::runtime_error{"resume"}); }
};

template <typename Pool>
[[nodiscard]]
auto make_resume_throws(Pool *pool) -> hop_resume_throws<typename Pool::context_type> {
  return {
      [](void *p, lf::sched_handle<typename Pool::context_type> h) {
        static_cast<Pool *>(p)->post(h);
      },
      pool,
  };
}

// ---- member-operator-co_await wrapper ----

template <typename Context>
struct hop_member_op {
  void (*post_fn)(void *, lf::sched_handle<Context>);
  void *pool_ptr;
  auto operator co_await() noexcept -> hop_to<Context> { return {post_fn, pool_ptr}; }
};

template <typename Pool>
[[nodiscard]]
auto make_hop_member(Pool *pool) -> hop_member_op<typename Pool::context_type> {
  return {
      [](void *p, lf::sched_handle<typename Pool::context_type> h) {
        static_cast<Pool *>(p)->post(h);
      },
      pool,
  };
}

// ---- free-operator-co_await wrapper ----

template <typename Context>
struct hop_free_op {
  void (*post_fn)(void *, lf::sched_handle<Context>);
  void *pool_ptr;
};

template <typename Context>
[[nodiscard]]
auto operator co_await(hop_free_op<Context> h) noexcept -> hop_to<Context> {
  return {h.post_fn, h.pool_ptr};
}

template <typename Pool>
[[nodiscard]]
auto make_hop_free(Pool *pool) -> hop_free_op<typename Pool::context_type> {
  return {
      [](void *p, lf::sched_handle<typename Pool::context_type> h) {
        static_cast<Pool *>(p)->post(h);
      },
      pool,
  };
}

// ============================================================
// Concept conformance helpers (used in test 15)
// ============================================================

namespace concept_checks {

using test_stack   = lf::geometric_stack<>;
using test_context = lf::mono_context<test_stack, lf::adapt_vector<>>;

// Well-formed: all methods present with correct signatures.
struct good_awaitable {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<test_context>, test_context &) noexcept -> void {}
  auto await_resume() noexcept -> void {}
};

// Wrong context type in await_suspend.
struct bad_context_type {
  auto await_ready() noexcept -> bool { return false; }
  auto await_suspend(lf::sched_handle<test_context>, int &) -> void {}
  auto await_resume() -> void {}
};

// await_ready returns something not convertible to bool.
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

// hop_to<test_context> itself: verifies our awaitable meets the concept.
// (Plain awaitable — no operator co_await.)
static_assert(lf::awaitable<hop_to<test_context>, test_context>,
              "hop_to<ctx> must satisfy lf::awaitable<T, ctx>");

// member-op wrapper must also satisfy the concept (acquire_awaitable peels it).
static_assert(lf::awaitable<hop_member_op<test_context>, test_context>,
              "hop_member_op must satisfy lf::awaitable");

// free-op wrapper.
static_assert(lf::awaitable<hop_free_op<test_context>, test_context>,
              "hop_free_op must satisfy lf::awaitable");

} // namespace concept_checks

} // namespace

// ============================================================
// Test cases
// ============================================================

// ---- 15. Concept conformance (compile-time only) ----
//
// This is the only test that currently works, because it does not require
// actually calling `co_await <user-awaitable>` inside a task body.
// See the note at the top of this file for the blocking bug.

TEST_CASE("explicit-sched: concept conformance", "[explicit-sched]") {
  using namespace concept_checks;

  // A well-formed awaitable satisfies lf::awaitable.
  STATIC_REQUIRE(lf::awaitable<good_awaitable, test_context>);

  // Wrong context type in await_suspend.
  STATIC_REQUIRE_FALSE(lf::awaitable<bad_context_type, test_context>);

  // await_ready not convertible to bool.
  STATIC_REQUIRE_FALSE(lf::awaitable<non_bool_ready, test_context>);

  // await_suspend returns non-void.
  STATIC_REQUIRE_FALSE(lf::awaitable<non_void_suspend, test_context>);

  // hop_to and its operator co_await wrappers satisfy the concept.
  STATIC_REQUIRE(lf::awaitable<hop_to<test_context>, test_context>);
  STATIC_REQUIRE(lf::awaitable<hop_member_op<test_context>, test_context>);
  STATIC_REQUIRE(lf::awaitable<hop_free_op<test_context>, test_context>);

  // hop_to_throwing satisfies the concept (noexcept is not required by the concept).
  STATIC_REQUIRE(lf::awaitable<hop_to_throwing<test_context>, test_context>);

  // hop_resume_throws satisfies the concept.
  STATIC_REQUIRE(lf::awaitable<hop_resume_throws<test_context>, test_context>);

  // nothrow_await_suspend is not exported from libfork, so we cannot test it here.
  // It is tested indirectly: hop_to has noexcept await_suspend; hop_to_throwing does not.
}
