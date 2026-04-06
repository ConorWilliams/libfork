#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

namespace {

// TODO: test exceptions

using lf::env;
using lf::task;

template <typename Context>
auto simple_function(env<Context>) -> task<bool, Context> {
  co_return true;
}

template <typename Context>
auto void_function(env<Context>) -> task<void, Context> {
  co_return;
}

template <typename Context>
auto throwing_function(env<Context>) -> task<void, Context> {
  LF_THROW(std::runtime_error{"This function always throws"});
  co_return;
}

} // namespace

TEST_CASE("Mono schedule", "[schedule]") {

  using context_type = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector>;

  STATIC_REQUIRE(lf::worker_context<context_type>);

  lf::inline_scheduler<context_type> scheduler;

  SECTION("void") {
    auto recv = schedule(scheduler, void_function<context_type>);
    REQUIRE(recv.valid());
    std::move(recv).get();
  }

  SECTION("non-void") {
    auto recv = schedule(scheduler, simple_function<context_type>);
    REQUIRE(recv.valid());
    REQUIRE(std::move(recv).get() == true);
  }

  SECTION("throwing") {
    auto recv = schedule(scheduler, throwing_function<context_type>);
    REQUIRE(recv.valid());
    REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
  }
}

TEST_CASE("Poly schedule", "[schedule]") {

  using derived_context = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector>;

  lf::inline_scheduler<derived_context> scheduler;

  using context_type = derived_context::context_type;

  STATIC_REQUIRE(lf::worker_context<context_type>);

  auto recv = schedule(scheduler, simple_function<context_type>);
  REQUIRE(recv.valid());
  REQUIRE(std::move(recv).get() == true);
}
