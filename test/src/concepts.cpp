#include <catch2/catch_test_macros.hpp>

import std;

import libfork;
import libfork.utils;

using namespace lf;

using test_stack = geometric_stack<>;
using test_context = mono_context<test_stack, adapt_vector<>>;

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
  STATIC_REQUIRE(specialization_of<task<int, test_context>, task>);

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
  STATIC_REQUIRE(worker_stack<test_stack>);

  struct bad_stack {
    struct ckpt {
      auto operator==(ckpt const &) const -> bool = default;
    };
    static auto push(std::size_t) -> void *;
    static auto pop(void *, std::size_t) -> void; // missing noexcept
    static auto checkpoint() noexcept -> ckpt;
    static auto prepare_release() noexcept -> int;
    static auto release(int) noexcept -> void;
    static auto acquire(ckpt const &) noexcept -> void;
  };

  STATIC_REQUIRE_FALSE(worker_stack<bad_stack>);
}

TEST_CASE("Concepts: worker_context", "[concepts]") {
  STATIC_REQUIRE(worker_context<test_context>);

  struct missing_push {
    auto pop() noexcept -> lf::steal_handle<missing_push>;
    auto stack() noexcept -> test_stack &;
  };

  STATIC_REQUIRE_FALSE(worker_context<missing_push>);
}

TEST_CASE("Concepts: async_invocable", "[concepts]") {

  auto async_fn_env(env<test_context>, int) -> task<int, test_context>;
  auto async_fn_no_env(int) -> task<int, test_context>;
  auto not_async_fn(int) -> int;

  struct both_invocable {
    auto operator()(env<test_context>, int) const -> task<int, test_context>;
    auto operator()(int) const -> task<double, test_context>;
  };

  // Basic positive cases
  STATIC_REQUIRE(async_invocable<decltype(async_fn_env), test_context, int>);
  STATIC_REQUIRE(async_invocable<decltype(async_fn_no_env), test_context, int>);

  // Arg mismatch
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_env), test_context, int *>);
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_no_env), test_context, double *>);

  // Result type check
  STATIC_REQUIRE(std::same_as<async_result_t<decltype(async_fn_env), test_context, int>, int>);

  // Preference check: when both are available, it should pick the one with env
  // and return int task.
  STATIC_REQUIRE(async_invocable_to<both_invocable, int, test_context, int>);
  // Verification that it didn't pick the double one
  STATIC_REQUIRE_FALSE(async_invocable_to<both_invocable, double, test_context, int>);

  // Fails unless return is a task
  STATIC_REQUIRE_FALSE(async_invocable<decltype(not_async_fn), test_context, int>);

  // Need a valid context of a different type
  struct mock_context {
    void push(lf::steal_handle<mock_context>);
    void post(lf::sched_handle<mock_context>);
    auto pop() noexcept -> lf::steal_handle<mock_context>;
    auto stack() noexcept -> test_stack &;
  };

  STATIC_REQUIRE(worker_context<mock_context>);

  // Fails because the result task's context doesn't match the provided context
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_no_env), mock_context, int>);
}

TEST_CASE("Concepts: async_nothrow_invocable", "[concepts]") {

  struct nothrow_async {
    auto operator()(int) const noexcept -> task<int, test_context>;
  };

  struct throwing_async {
    auto operator()(int) const -> task<int, test_context>;
  };

  STATIC_REQUIRE(async_nothrow_invocable<nothrow_async, test_context, int>);
  STATIC_REQUIRE_FALSE(async_nothrow_invocable<throwing_async, test_context, int>);
}

namespace {

struct plain_awaitable {
  auto await_ready() -> bool;
  auto await_suspend(std::coroutine_handle<>) -> void;
  auto await_resume() -> void;
};

struct member_co_await {
  auto operator co_await() -> plain_awaitable;
};

struct free_co_await {
  friend auto operator co_await(free_co_await) -> plain_awaitable { return {}; }
};

struct both_co_await {
  auto operator co_await() -> plain_awaitable;
  friend auto operator co_await(both_co_await) -> plain_awaitable { return {}; }
};

template <typename T>
concept can_acquire = requires (T &&t) { lf::acquire_awaitable(static_cast<T &&>(t)); };

} // namespace

TEST_CASE("Concepts: awaitable_acquirable", "[concepts]") {
  // Generic identity overload accepts any type — even non-awaiters.
  STATIC_REQUIRE(can_acquire<plain_awaitable>);
  STATIC_REQUIRE(can_acquire<int>);

  // A single operator co_await — member or free — disambiguates the dispatch.
  STATIC_REQUIRE(can_acquire<member_co_await>);
  STATIC_REQUIRE(can_acquire<free_co_await>);

  // Defining BOTH member and free operator co_await leaves the dispatch ambiguous.
  STATIC_REQUIRE_FALSE(can_acquire<both_co_await>);
}

TEST_CASE("acquire_awaitable", "[concepts]") {
  // No operator co_await: returns the argument unchanged, preserving value category.
  STATIC_REQUIRE(std::same_as<decltype(acquire_awaitable(std::declval<plain_awaitable>())), plain_awaitable &&>);
  STATIC_REQUIRE(std::same_as<decltype(acquire_awaitable(std::declval<plain_awaitable &>())), plain_awaitable &>);
  STATIC_REQUIRE(
      std::same_as<decltype(acquire_awaitable(std::declval<plain_awaitable const &>())), plain_awaitable const &>
  );

  // Member operator co_await: returns whatever the member call produces.
  STATIC_REQUIRE(std::same_as<decltype(acquire_awaitable(std::declval<member_co_await>())), plain_awaitable>);

  // Free operator co_await: returns whatever the ADL-found free call produces.
  STATIC_REQUIRE(std::same_as<decltype(acquire_awaitable(std::declval<free_co_await>())), plain_awaitable>);
}
