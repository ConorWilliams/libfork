#include <catch2/catch_test_macros.hpp>

import std;

import libfork.core;

// TODO: test (we need a context...)

namespace {

using lf::env;
using lf::task;

template <typename Context>
auto void_function(env<Context>) -> task<void, Context> {
  co_return;
}

} // namespace

TEST_CASE("Simple schedule", "[schedule]") {

  lf::generic_context<false, lf::stacks::geometric<>> context; //
}
