#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

namespace {

using lf::env;
using lf::task;

template <typename Context>
auto simple_function(env<Context> /*unused*/) -> task<bool, Context> {
  co_return true;
}

template <typename Context>
auto void_function(env<Context> /*unused*/) -> task<void, Context> {
  co_return;
}

template <typename Context>
auto throwing_function(env<Context> /*unused*/) -> task<void, Context> {
  LF_THROW(std::runtime_error{"This function always throws"});
  co_return;
}

// Task whose argument is a big enough value-type to push the root coroutine
// frame past the 1 KiB embedded buffer in receiver_state.
template <typename Context>
auto big_arg_function(env<Context> /*unused*/, std::array<std::byte, 2048> /*unused*/)
    -> task<void, Context> {
  co_return;
}

template <typename Sch>
void simple_tests(Sch &scheduler) {
  SECTION("void") {
    auto recv = schedule(scheduler, void_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("non-void") {
    auto recv = schedule(scheduler, simple_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == true);
  }

  SECTION("explicit root_state") {
    lf::root_state<bool, false> state;
    auto recv = schedule(scheduler, std::move(state), simple_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == true);
  }

  SECTION("stoppable root_state") {
    lf::root_state<bool, true> state;
    auto recv = schedule(scheduler, std::move(state), simple_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == true);
  }

  SECTION("root_state with explicit allocator") {
    std::allocator<std::byte> alloc;
    lf::root_state<bool, false> state{std::allocator_arg, alloc};
    auto recv = schedule(scheduler, std::move(state), simple_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == true);
  }

  SECTION("root_state with value-init") {
    // Pre-initialise the return slot; the task overwrites it.
    lf::root_state<bool, false> state{false};
    auto recv = schedule(scheduler, std::move(state), simple_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == true);
  }

#if LF_COMPILER_EXCEPTIONS
  SECTION("throwing") {
    auto recv = schedule(scheduler, throwing_function<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
  }

  SECTION("frame too large -> root_alloc_error") {
    std::array<std::byte, 2048> big{};
    REQUIRE_THROWS_AS(schedule(scheduler, big_arg_function<lf::context_t<Sch>>, big),
                      lf::root_alloc_error);
  }
#endif
}

using mono_inline_ctx = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector>;
using poly_inline_ctx = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector>;

} // namespace

TEMPLATE_TEST_CASE("Inline schedule", "[schedule]", mono_inline_ctx, poly_inline_ctx) {
  lf::inline_scheduler<TestType> scheduler;
  simple_tests(scheduler);
}

namespace {

using mono_busy_thread_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_busy_thread_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

} // namespace

TEMPLATE_TEST_CASE("Busy schedule", "[schedule]", mono_busy_thread_pool, poly_busy_thread_pool) {

  STATIC_REQUIRE(lf::scheduler<TestType>);

  for (std::size_t thr = 1; thr < 4; ++thr) {
    TestType scheduler{thr};
    simple_tests(scheduler);
  }
}
