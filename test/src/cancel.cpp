#include <catch2/catch_test_macros.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

namespace {

struct add_one {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::atomic<int> &count) -> lf::task<int, Context> {
    co_return count.fetch_add(1);
  }
};

template <typename Context>
auto test_cancel(lf::env<Context>) -> lf::task<bool, Context> {

  // Test that a task that a call doesn't run if cancelled

  std::atomic<int> count = 0;

  auto stop_src = co_await lf::child_stop_source();
  auto sc = co_await lf::scope();

  stop_src.request_stop();

  co_await sc.call_with_drop(&stop_src, add_one{}, count);
  co_await lf::join();

  co_return count.load() == 0;
}

auto test_no_join() {}

template <typename Sch>
void tests(Sch &scheduler) {
  SECTION("Canceled is not run") {
    auto recv = schedule(scheduler, test_cancel<lf::context_t<Sch>>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get());
  }

  // #if LF_COMPILER_EXCEPTIONS
  //   SECTION("throwing") {
  //     auto recv = schedule(scheduler, throwing_function<lf::context_t<Sch>>);
  //     REQUIRE(recv.valid());
  //     REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
  //   }
  // #endif
}

using mono_inline_ctx = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector>;
using poly_inline_ctx = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector>;

} // namespace

TEMPLATE_TEST_CASE("Innline cancel", "[cancel]", mono_inline_ctx, poly_inline_ctx) {
  lf::inline_scheduler<TestType> sch{};
  tests(sch);
}

// namespace {
//
// using mono_busy_thread_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
// using poly_busy_thread_pool = lf::poly_busy_pool<lf::geometric_stack<>>;
//
// } // namespace
//
// TEMPLATE_TEST_CASE("Busy schedule", "[schedule]", mono_busy_thread_pool, poly_busy_thread_pool) {
//
//   STATIC_REQUIRE(lf::scheduler<TestType>);
//
//   for (std::size_t thr = 1; thr < 4; ++thr) {
//     TestType scheduler{thr};
//     simple_tests(scheduler);
//   }
// }
// mport libfork;
