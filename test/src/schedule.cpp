#include <catch2/catch_test_macros.hpp>

import std;

import libfork;

namespace {

// TODO: test exceptions

using lf::env;
using lf::task;

template <typename Context>
auto void_function(env<Context>) -> task<bool, Context> {
  co_return true;
}

} // namespace

TEST_CASE("Simple schedule", "[schedule]") {

  using context_type = lf::mono_context<lf::stacks::geometric<>, lf::adapt_vector>;

  STATIC_REQUIRE(lf::worker_context<context_type>);

  lf::inline_scheduler<context_type> scheduler;

  auto recv = schedule(scheduler, void_function<context_type>);
  REQUIRE(recv.valid());
  REQUIRE(std::move(recv).get() == true);
}

TEST_CASE("Poly schedule", "[schedule]") {

  using derived_context = lf::derived_poly_context<lf::stacks::geometric<>, lf::adapt_vector>;

  lf::inline_scheduler<derived_context> scheduler;

  using context_type = derived_context::context_type;

  STATIC_REQUIRE(lf::worker_context<context_type>);

  auto recv = schedule(scheduler, void_function<context_type>);
  REQUIRE(recv.valid());
  REQUIRE(std::move(recv).get() == true);
}
