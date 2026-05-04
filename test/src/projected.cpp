#include <catch2/catch_test_macros.hpp>

import std;

import libfork;

namespace {

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using context_t = mono_pool::context_type;

// Sync projection: int -> double
struct sync_proj {
  auto operator()(int const &) const -> double;
};

// Async projection: int -> task<double, Context>
struct async_proj {
  template <typename Context>
  static auto operator()(lf::env<Context>, int const &) -> lf::task<double, Context>;
};

using iter_t = std::vector<int>::iterator;

// Sync: lf::projected<I, sync_proj, Ctx> ≡ std::projected<I, sync_proj> in shape.
using sync_projected = lf::projected<iter_t, sync_proj, context_t>;

static_assert(std::same_as<sync_projected::value_type, double>);
static_assert(std::same_as<sync_projected::difference_type, std::iter_difference_t<iter_t>>);
static_assert(std::indirectly_readable<sync_projected>);
static_assert(std::same_as<std::iter_reference_t<sync_projected>, double>);

// Async: dereference yields the task's value_type (after awaiting).
using async_projected = lf::projected<iter_t, async_proj, context_t>;

static_assert(std::same_as<async_projected::value_type, double>);
static_assert(std::same_as<async_projected::difference_type, std::iter_difference_t<iter_t>>);
static_assert(std::indirectly_readable<async_projected>);
static_assert(std::same_as<std::iter_reference_t<async_projected>, double>);

// Concept selection: indirectly_unary_invocable<F, Ctx, I> works for
// both a sync lambda-equivalent and an async function.
struct sync_fn {
  void operator()(int &) const;
};

struct async_fn {
  template <typename Context>
  static auto operator()(lf::env<Context>, int &) -> lf::task<void, Context>;
};

static_assert(lf::indirectly_unary_invocable<sync_fn, context_t, iter_t>);
static_assert(lf::indirectly_unary_invocable<async_fn, context_t, iter_t>);

// A function that's neither must fail.
struct not_invocable {};
static_assert(!lf::indirectly_unary_invocable<not_invocable, context_t, iter_t>);

// Without difference_type when I is not weakly_incrementable.
struct readable_only {
  using value_type = int;
  auto operator*() const -> int &;
};
static_assert(std::indirectly_readable<readable_only>);
static_assert(!std::weakly_incrementable<readable_only>);

template <typename T>
concept has_difference_type = requires { typename T::difference_type; };

using projected_no_diff = lf::projected<readable_only, sync_proj, context_t>;
static_assert(std::same_as<projected_no_diff::value_type, double>);
static_assert(!has_difference_type<projected_no_diff>);
static_assert(has_difference_type<sync_projected>);

} // namespace

TEST_CASE("projected: compile-only checks", "[projected]") {
  SUCCEED("static_asserts above");
}
