#include <catch2/catch_test_macros.hpp>

import std;

import libfork;
import libfork.utils;

using namespace lf;

TEST_CASE("Concepts: atomicable", "[concepts]") {
  STATIC_REQUIRE(atomicable<std::byte>);
  STATIC_REQUIRE(atomicable<void *>);

  struct trivial {
    int x;
    float y;
  };

  STATIC_REQUIRE(atomicable<trivial>);

  STATIC_REQUIRE_FALSE(atomicable<std::string>);
  STATIC_REQUIRE_FALSE(atomicable<const int>);
  STATIC_REQUIRE_FALSE(atomicable<int &>);
}

TEST_CASE("Concepts: lock_free", "[concepts]") {
  STATIC_REQUIRE(lock_free<std::byte>);
  STATIC_REQUIRE(lock_free<void *>);
}

namespace {

template <typename... T>
struct my_template {};

} // namespace

TEST_CASE("Concepts: specialization_of", "[concepts]") {
  STATIC_REQUIRE(specialization_of<std::vector<int>, std::vector>);
  STATIC_REQUIRE(specialization_of<my_template<int, float>, my_template>);
  STATIC_REQUIRE(specialization_of<task<int, dummy_context>, task>);

  STATIC_REQUIRE_FALSE(specialization_of<int, std::vector>);
  STATIC_REQUIRE_FALSE(specialization_of<std::vector<int>, my_template>);
}

TEST_CASE("Concepts: returnable", "[concepts]") {
  STATIC_REQUIRE(returnable<void>);
  STATIC_REQUIRE(returnable<int>);
  STATIC_REQUIRE(returnable<std::unique_ptr<int>>);

  struct non_movable {
    non_movable() = default;
    non_movable(const non_movable &) = delete;
    non_movable(non_movable &&) = delete;
  };

  STATIC_REQUIRE_FALSE(returnable<non_movable>);
}

TEST_CASE("Concepts: worker_stack", "[concepts]") {
  STATIC_REQUIRE(worker_stack<dummy_allocator>);

  struct bad_alloc : dummy_allocator {
    constexpr static auto pop(void *p, std::size_t sz) -> void;
  };

  STATIC_REQUIRE_FALSE(worker_stack<bad_alloc>);
}

TEST_CASE("Concepts: worker_context", "[concepts]") {
  STATIC_REQUIRE(worker_context<dummy_context>);

  struct missing_push {
    auto pop() noexcept -> lf::steal_handle<missing_push>;
    auto stack() noexcept -> dummy_allocator &;
  };

  STATIC_REQUIRE_FALSE(worker_context<missing_push>);
}

TEST_CASE("Concepts: async_invocable", "[concepts]") {

  auto async_fn_env(env<dummy_context>, int) -> task<int, dummy_context>;
  auto async_fn_no_env(int) -> task<int, dummy_context>;
  auto not_async_fn(int) -> int;

  struct both_invocable {
    auto operator()(env<dummy_context>, int) const -> task<int, dummy_context>;
    auto operator()(int) const -> task<double, dummy_context>;
  };

  // Basic positive cases
  STATIC_REQUIRE(async_invocable<decltype(async_fn_env), dummy_context, int>);
  STATIC_REQUIRE(async_invocable<decltype(async_fn_no_env), dummy_context, int>);

  // Arg mismatch
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_env), dummy_context, int *>);
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_no_env), dummy_context, double *>);

  // Result type check
  STATIC_REQUIRE(std::same_as<async_result_t<decltype(async_fn_env), dummy_context, int>, int>);

  // Preference check: when both are available, it should pick the one with env
  // and return int task.
  STATIC_REQUIRE(async_invocable_to<both_invocable, int, dummy_context, int>);
  // Verification that it didn't pick the double one
  STATIC_REQUIRE_FALSE(async_invocable_to<both_invocable, double, dummy_context, int>);

  // Fails unless return is a task
  STATIC_REQUIRE_FALSE(async_invocable<decltype(not_async_fn), dummy_context, int>);

  // Need a valid context of a different type
  struct mock_context {
    void push(lf::steal_handle<mock_context>);
    void post(lf::sched_handle<mock_context>);
    auto pop() noexcept -> lf::steal_handle<mock_context>;
    auto stack() noexcept -> dummy_allocator &;
  };

  STATIC_REQUIRE(worker_context<mock_context>);

  // Fails because the result task's context doesn't match the provided context
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_no_env), mock_context, int>);
}

TEST_CASE("Concepts: async_nothrow_invocable", "[concepts]") {

  struct nothrow_async {
    auto operator()(int) const noexcept -> task<int, dummy_context>;
  };

  struct throwing_async {
    auto operator()(int) const -> task<int, dummy_context>;
  };

  STATIC_REQUIRE(async_nothrow_invocable<nothrow_async, dummy_context, int>);
  STATIC_REQUIRE_FALSE(async_nothrow_invocable<throwing_async, dummy_context, int>);
}
