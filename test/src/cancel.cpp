#include <catch2/catch_test_macros.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

namespace {

template <typename Context>
auto add_one(lf::env<Context>, std::atomic<int> *count) -> lf::task<int, Context> {
  co_return count->fetch_add(1);
}

// template <typename Context>
// auto test_cancel(lf::env<Context>) -> lf::task<void, Context> {
//
//   lf::stop_source src;
//
//   co_return;
// }

template <typename Sch>
void simple_tests(Sch &scheduler) {
  SECTION("void") {

    std::atomic<int> count = 0;

    auto recv = schedule(scheduler, add_one<lf::context_t<Sch>>, &count);
    REQUIRE(recv.valid());
    std::move(recv).get();
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
  simple_tests(sch);
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
