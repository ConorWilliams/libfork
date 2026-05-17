#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

namespace {

using mono_inline_ctx = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector<>>;
using poly_inline_ctx = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector<>>;
using mono_inline_scheduler = lf::inline_scheduler<mono_inline_ctx>;
using poly_inline_scheduler = lf::inline_scheduler<poly_inline_ctx>;
using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

template <typename Scheduler>
[[nodiscard]]
auto make_scheduler() -> Scheduler {
  if constexpr (std::constructible_from<Scheduler, std::size_t>) {
    return Scheduler{2};
  } else {
    return Scheduler{};
  }
}

struct plus_fn {
  [[nodiscard]]
  constexpr auto operator()(int lhs, int rhs) const noexcept -> int {
    return lhs + rhs;
  }
};

struct add_to_atomic {
  void operator()(std::atomic<int> *value, int delta) const noexcept {
    value->fetch_add(delta, std::memory_order_relaxed);
  }
};

struct add_to_atomic_return {
  [[nodiscard]]
  auto operator()(std::atomic<int> *value, int delta) const noexcept -> int {
    value->fetch_add(delta, std::memory_order_relaxed);
    return delta;
  }
};

struct append_value {
  void operator()(std::vector<int> &values, int value) const { values.push_back(value); }
};

struct take_unique {
  [[nodiscard]]
  auto operator()(std::unique_ptr<int> value) const noexcept -> int {
    return *value;
  }
};

struct move_only_plus {
  std::unique_ptr<int> bias;

  explicit move_only_plus(int value)
      : bias(std::make_unique<int>(value)) {}

  move_only_plus(move_only_plus &&) noexcept = default;
  auto operator=(move_only_plus &&) noexcept -> move_only_plus & = default;
  move_only_plus(move_only_plus const &) = delete;
  auto operator=(move_only_plus const &) -> move_only_plus & = delete;

  [[nodiscard]]
  auto operator()(int value) && noexcept -> int {
    return value + *bias;
  }
};

struct multiplier {
  int factor;

  [[nodiscard]]
  auto apply(int value) const noexcept -> int {
    return factor * value;
  }
};

struct run_lift_scope_ops {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<bool, Context> {
    bool ok = true;
    int call_result = 0;
    int fork_result = 0;
    int move_result = 0;
    int member_result = 0;
    std::atomic<int> effects{0};
    std::vector<int> values;

    auto sc = co_await lf::scope();

    co_await sc.call(&call_result, lf::lift, plus_fn{}, 2, 3);
    ok = ok && call_result == 5;

    co_await sc.call(lf::lift, append_value{}, values, 7);
    ok = ok && values == std::vector{7};

    co_await sc.call_drop(lf::lift, add_to_atomic_return{}, &effects, 11);
    ok = ok && effects.load(std::memory_order_relaxed) == 11;

    co_await sc.call_drop(lf::lift, add_to_atomic{}, &effects, 13);
    ok = ok && effects.load(std::memory_order_relaxed) == 24;

    co_await sc.call(&move_result, lf::lift, take_unique{}, std::make_unique<int>(17));
    ok = ok && move_result == 17;

    co_await sc.call(&member_result, lf::lift, &multiplier::apply, multiplier{3}, 5);
    ok = ok && member_result == 15;

    co_await sc.fork(&fork_result, lf::lift, plus_fn{}, 19, 23);
    co_await sc.fork(lf::lift, add_to_atomic{}, &effects, 29);
    co_await sc.fork_drop(lf::lift, add_to_atomic_return{}, &effects, 31);
    co_await sc.fork_drop(lf::lift, add_to_atomic{}, &effects, 37);
    co_await sc.join();

    ok = ok && fork_result == 42;
    ok = ok && effects.load(std::memory_order_relaxed) == 121;

    co_await sc.fork(&move_result, lf::lift, take_unique{}, std::make_unique<int>(41));
    co_await sc.fork(&member_result, lf::lift, &multiplier::apply, multiplier{7}, 6);
    co_await sc.fork(&call_result, lf::lift, move_only_plus{5}, 8);
    co_await sc.join();

    ok = ok && move_result == 41;
    ok = ok && member_result == 42;
    ok = ok && call_result == 13;

    co_return ok;
  }
};

struct run_child_lift_scope_ops {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<bool, Context> {
    bool ok = true;
    int call_result = 0;
    int fork_result = 0;
    int move_result = 0;
    int member_result = 0;
    std::atomic<int> effects{0};
    std::vector<int> values;

    auto sc = co_await lf::child_scope();

    co_await sc.call(&call_result, lf::lift, plus_fn{}, 2, 3);
    ok = ok && call_result == 5;

    co_await sc.call(lf::lift, append_value{}, values, 7);
    ok = ok && values == std::vector{7};

    co_await sc.call_drop(lf::lift, add_to_atomic_return{}, &effects, 11);
    ok = ok && effects.load(std::memory_order_relaxed) == 11;

    co_await sc.call_drop(lf::lift, add_to_atomic{}, &effects, 13);
    ok = ok && effects.load(std::memory_order_relaxed) == 24;

    co_await sc.call(&move_result, lf::lift, take_unique{}, std::make_unique<int>(17));
    ok = ok && move_result == 17;

    co_await sc.call(&member_result, lf::lift, &multiplier::apply, multiplier{3}, 5);
    ok = ok && member_result == 15;

    co_await sc.fork(&fork_result, lf::lift, plus_fn{}, 19, 23);
    co_await sc.fork(lf::lift, add_to_atomic{}, &effects, 29);
    co_await sc.fork_drop(lf::lift, add_to_atomic_return{}, &effects, 31);
    co_await sc.fork_drop(lf::lift, add_to_atomic{}, &effects, 37);
    co_await sc.join();

    ok = ok && fork_result == 42;
    ok = ok && effects.load(std::memory_order_relaxed) == 121;

    co_await sc.fork(&move_result, lf::lift, take_unique{}, std::make_unique<int>(41));
    co_await sc.fork(&member_result, lf::lift, &multiplier::apply, multiplier{7}, 6);
    co_await sc.fork(&call_result, lf::lift, move_only_plus{5}, 8);
    co_await sc.join();

    ok = ok && move_result == 41;
    ok = ok && member_result == 42;
    ok = ok && call_result == 13;

    co_return ok;
  }
};

struct run_cancelled_child_lift_scope_ops {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<bool, Context> {
    int call_result = 101;
    int fork_result = 202;
    std::atomic<int> effects{0};

    auto sc = co_await lf::child_scope();
    sc.request_stop();

    co_await sc.call(&call_result, lf::lift, add_to_atomic_return{}, &effects, 1);
    co_await sc.call(lf::lift, add_to_atomic{}, &effects, 2);
    co_await sc.call_drop(lf::lift, add_to_atomic_return{}, &effects, 4);
    co_await sc.call_drop(lf::lift, add_to_atomic{}, &effects, 8);

    co_await sc.fork(&fork_result, lf::lift, add_to_atomic_return{}, &effects, 16);
    co_await sc.fork(lf::lift, add_to_atomic{}, &effects, 32);
    co_await sc.fork_drop(lf::lift, add_to_atomic_return{}, &effects, 64);
    co_await sc.fork_drop(lf::lift, add_to_atomic{}, &effects, 128);
    co_await sc.join();

    co_return call_result == 101 && fork_result == 202 && effects.load(std::memory_order_relaxed) == 0;
  }
};

#if LF_COMPILER_EXCEPTIONS

struct throwing_sync {
  [[noreturn]]
  auto operator()() const -> int {
    LF_THROW(std::runtime_error{"lift"});
  }
};

struct run_call_lift_exception {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    int result = 0;
    auto sc = co_await lf::scope();
    co_await sc.call(&result, lf::lift, throwing_sync{});
    co_await sc.join();
    co_return;
  }
};

struct run_fork_lift_exception {
  template <typename Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    int result = 0;
    auto sc = co_await lf::scope();
    co_await sc.fork(&result, lf::lift, throwing_sync{});
    co_await sc.join();
    co_return;
  }
};

#endif // LF_COMPILER_EXCEPTIONS

template <typename Scheduler>
void check_lift_scope_ops(Scheduler &scheduler) {
  auto scope_recv = lf::schedule(scheduler, run_lift_scope_ops{});
  REQUIRE(std::move(scope_recv).get());

  auto child_scope_recv = lf::schedule(scheduler, run_child_lift_scope_ops{});
  REQUIRE(std::move(child_scope_recv).get());

  auto cancelled_recv = lf::schedule(scheduler, run_cancelled_child_lift_scope_ops{});
  REQUIRE(std::move(cancelled_recv).get());
}

} // namespace

TEST_CASE("lift: concept conformance", "[lift]") {
  STATIC_REQUIRE(lf::async_invocable<decltype(lf::lift), mono_inline_ctx, plus_fn, int, int>);
  STATIC_REQUIRE(lf::async_invocable_to<decltype(lf::lift), int, mono_inline_ctx, plus_fn, int, int>);
  STATIC_REQUIRE(lf::async_invocable_to<decltype(lf::lift),
                                        void,
                                        mono_inline_ctx,
                                        add_to_atomic,
                                        std::atomic<int> *,
                                        int>);
  STATIC_REQUIRE(
      lf::async_invocable_to<decltype(lf::lift), int, mono_inline_ctx, take_unique, std::unique_ptr<int>>);
  STATIC_REQUIRE(lf::async_invocable_to<decltype(lf::lift), int, mono_inline_ctx, move_only_plus, int>);
  STATIC_REQUIRE(lf::async_invocable_to<decltype(lf::lift),
                                        int,
                                        mono_inline_ctx,
                                        int (multiplier::*)(int) const,
                                        multiplier,
                                        int>);

  STATIC_REQUIRE_FALSE(lf::async_invocable<plus_fn, mono_inline_ctx, int, int>);
  STATIC_REQUIRE_FALSE(lf::async_invocable_to<decltype(lf::lift),
                                              int,
                                              mono_inline_ctx,
                                              add_to_atomic,
                                              std::atomic<int> *,
                                              int>);
}

TEMPLATE_TEST_CASE(
    "lift: direct schedule", "[lift]", mono_inline_scheduler, poly_inline_scheduler, mono_pool, poly_pool) {
  auto scheduler = make_scheduler<TestType>();

  auto value_recv = lf::schedule(scheduler, lf::lift, plus_fn{}, 20, 22);
  REQUIRE(std::move(value_recv).get() == 42);

  std::atomic<int> effect{0};
  auto void_recv = lf::schedule(scheduler, lf::lift, add_to_atomic{}, &effect, 17);
  std::move(void_recv).get();
  REQUIRE(effect.load(std::memory_order_relaxed) == 17);

  auto move_recv = lf::schedule(scheduler, lf::lift, take_unique{}, std::make_unique<int>(29));
  REQUIRE(std::move(move_recv).get() == 29);
}

TEMPLATE_TEST_CASE(
    "lift: scope operations", "[lift]", mono_inline_scheduler, poly_inline_scheduler, mono_pool, poly_pool) {
  auto scheduler = make_scheduler<TestType>();
  check_lift_scope_ops(scheduler);
}

#if LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE("lift: exceptions propagate",
                   "[lift]",
                   mono_inline_scheduler,
                   poly_inline_scheduler,
                   mono_pool,
                   poly_pool) {
  auto scheduler = make_scheduler<TestType>();

  auto call_recv = lf::schedule(scheduler, run_call_lift_exception{});
  REQUIRE_THROWS_AS(std::move(call_recv).get(), std::runtime_error);

  auto fork_recv = lf::schedule(scheduler, run_fork_lift_exception{});
  REQUIRE_THROWS_AS(std::move(fork_recv).get(), std::runtime_error);
}

#endif // LF_COMPILER_EXCEPTIONS
