#include <catch2/catch_test_macros.hpp>

import std;

import libfork.core;
import libfork.schedule;

// TODO: test (we need a context...)

namespace {

using lf::env;
using lf::task;

template <typename Context>
auto void_function(env<Context>) -> task<bool, Context> {
  co_return true;
}

} // namespace

TEST_CASE("Simple schedule", "[schedule]") {

  using context_type = lf::generic_context<false, lf::stacks::geometric<>>;

  // lf::generic_context<false, lf::stacks::geometric<>> context; //

  lf::inline_scheduler<context_type> scheduler;

  auto recv = schedule2(scheduler, void_function<context_type>);

  REQUIRE(recv.valid());

  REQUIRE(std::move(recv).get() == true);
}
