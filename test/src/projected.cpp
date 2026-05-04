#include <catch2/catch_test_macros.hpp>

import std;

import libfork;

namespace {

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using context_t = mono_pool::context_type;

using vec_iter = std::vector<int>::iterator;
using cvec_iter = std::vector<int>::const_iterator;

// ============================================================================
// Projection function objects (I -> R)
// ============================================================================

struct sync_proj {
  auto operator()(int const &) const -> double;
};

struct async_proj {
  template <typename Context>
  static auto operator()(lf::env<Context>, int const &) -> lf::task<double, Context>;
};

// Both sync- and async-invocable; sync wins the for_each `if constexpr` branch
// and the disjunction is satisfied either way.
struct hybrid_proj {
  auto operator()(int const &) const -> double;
  template <typename Context>
  static auto operator()(lf::env<Context>, int const &) -> lf::task<double, Context>;
};

// Reference-returning sync projection — value_type strips cvref, deref keeps it.
// (The async equivalent is not testable: `task<T, ...>` requires `returnable<T>`,
// which forbids references.)
struct sync_ref_proj {
  auto operator()(int &) const -> int &;
};

// Returns different types depending on lvalue vs rvalue invocation.
// `std::projected` and (correctly) `lf::projected` invoke through `Proj &`, so
// `value_type` and `iter_reference_t` must both be `double`. If the impl uses
// `Proj` (rvalue) instead, `value_type` becomes `int`.
struct dual_qualified_proj {
  auto operator()(int const &) & -> double;
  auto operator()(int const &) && -> int;
};

// Both sync- and async-invocable, with *different* return types.
// `invoke_result`'s constrained partial specialization picks async, so the
// projected's `value_type` must be `double` (the async result), not `long`.
struct dual_mode_proj {
  auto operator()(int const &) const -> long;
  template <typename Context>
  static auto operator()(lf::env<Context>, int const &) -> lf::task<double, Context>;
};

// double -> std::string, used for nesting tests.
struct str_proj {
  auto operator()(double const &) const -> std::string;
};

struct async_str_proj {
  template <typename Context>
  static auto operator()(lf::env<Context>, double const &) -> lf::task<std::string, Context>;
};

// ============================================================================
// Consumer function objects (for `indirectly_unary_invocable`)
// ============================================================================

struct sync_fn {
  void operator()(int &) const;
};

struct async_fn {
  template <typename Context>
  static auto operator()(lf::env<Context>, int &) -> lf::task<void, Context>;
};

struct hybrid_fn {
  void operator()(int &) const;
  template <typename Context>
  static auto operator()(lf::env<Context>, int &) -> lf::task<void, Context>;
};

struct not_invocable {};

struct binary_fn {
  void operator()(int &, int &) const;
};

struct wrong_arg_fn {
  void operator()(double &) const;
};

struct async_fn_no_copy {
  async_fn_no_copy() = default;
  async_fn_no_copy(async_fn_no_copy const &) = delete;
  async_fn_no_copy(async_fn_no_copy &&) = default;
  template <typename Context>
  static auto operator()(lf::env<Context>, int &) -> lf::task<void, Context>;
};

struct bad_ctx {};

struct readable_only {
  using value_type = int;
  auto operator*() const -> int &;
};

template <typename T>
concept has_difference_type = requires { typename T::difference_type; };

// ============================================================================
// Type aliases under test
// ============================================================================

using sync_projected = lf::projected<vec_iter, sync_proj, context_t>;
using async_projected = lf::projected<vec_iter, async_proj, context_t>;
using hybrid_projected = lf::projected<vec_iter, hybrid_proj, context_t>;
using sync_cprojected = lf::projected<cvec_iter, sync_proj, context_t>;
using sync_ref_projected = lf::projected<vec_iter, sync_ref_proj, context_t>;
using projected_no_diff = lf::projected<readable_only, sync_proj, context_t>;
using dual_qualified_projected = lf::projected<vec_iter, dual_qualified_proj, context_t>;
using dual_mode_projected = lf::projected<vec_iter, dual_mode_proj, context_t>;

using sync_then_sync = lf::projected<sync_projected, str_proj, context_t>;
using async_then_async = lf::projected<async_projected, async_str_proj, context_t>;
using sync_then_async = lf::projected<sync_projected, async_str_proj, context_t>;
using async_then_sync = lf::projected<async_projected, str_proj, context_t>;

} // namespace

TEST_CASE("projected: value_type, difference_type, dereference", "[projected]") {
  STATIC_REQUIRE(std::same_as<sync_projected::value_type, double>);
  STATIC_REQUIRE(std::same_as<async_projected::value_type, double>);
  STATIC_REQUIRE(std::same_as<hybrid_projected::value_type, double>);

  STATIC_REQUIRE(std::same_as<sync_projected::difference_type, std::iter_difference_t<vec_iter>>);
  STATIC_REQUIRE(std::same_as<async_projected::difference_type, std::iter_difference_t<vec_iter>>);
  STATIC_REQUIRE(std::same_as<hybrid_projected::difference_type, std::iter_difference_t<vec_iter>>);

  STATIC_REQUIRE(std::indirectly_readable<sync_projected>);
  STATIC_REQUIRE(std::indirectly_readable<async_projected>);
  STATIC_REQUIRE(std::indirectly_readable<hybrid_projected>);

  STATIC_REQUIRE(std::same_as<std::iter_reference_t<sync_projected>, double>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<async_projected>, double>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<hybrid_projected>, double>);

  STATIC_REQUIRE(std::same_as<std::iter_value_t<sync_projected>, double>);
  STATIC_REQUIRE(std::same_as<std::iter_value_t<async_projected>, double>);
}

TEST_CASE("projected: const-iterator source", "[projected]") {
  STATIC_REQUIRE(std::same_as<sync_cprojected::value_type, double>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<sync_cprojected>, double>);
}

TEST_CASE("projected: reference-returning projection", "[projected]") {
  STATIC_REQUIRE(std::same_as<sync_ref_projected::value_type, int>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<sync_ref_projected>, int &>);
}

TEST_CASE("projected: invokes through Proj& (matches std::projected)", "[projected]") {
  // `operator() &` returns double; `operator() &&` returns int. Both
  // `std::projected::value_type` and `operator*()` are computed via lvalue
  // invocation, so the result must be `double` here.
  STATIC_REQUIRE(std::same_as<dual_qualified_projected::value_type, double>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<dual_qualified_projected>, double>);
}

TEST_CASE("projected: async invocation takes precedence over sync", "[projected]") {
  // dual_mode_proj is invocable both ways: sync returns `long`, async returns `double`.
  // `invoke_result`'s constrained partial specialization (gated on async_invocable)
  // is more constrained than the primary, so the async branch wins.
  STATIC_REQUIRE(std::indirectly_unary_invocable<dual_mode_proj, vec_iter>);
  STATIC_REQUIRE(lf::indirectly_unary_async_invocable<dual_mode_proj, context_t, vec_iter>);

  STATIC_REQUIRE(std::same_as<dual_mode_projected::value_type, double>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<dual_mode_projected>, double>);
}

TEST_CASE("projected: difference_type only when source is weakly_incrementable", "[projected]") {
  STATIC_REQUIRE(std::indirectly_readable<readable_only>);
  STATIC_REQUIRE_FALSE(std::weakly_incrementable<readable_only>);

  STATIC_REQUIRE(std::same_as<projected_no_diff::value_type, double>);
  STATIC_REQUIRE_FALSE(has_difference_type<projected_no_diff>);
  STATIC_REQUIRE(has_difference_type<sync_projected>);
  STATIC_REQUIRE(has_difference_type<async_projected>);
}

TEST_CASE("indirectly_unary_invocable: sync, async, hybrid", "[projected]") {
  STATIC_REQUIRE(lf::indirectly_unary_invocable<sync_fn, context_t, vec_iter>);
  STATIC_REQUIRE(lf::indirectly_unary_invocable<async_fn, context_t, vec_iter>);
  STATIC_REQUIRE(lf::indirectly_unary_invocable<hybrid_fn, context_t, vec_iter>);

  STATIC_REQUIRE_FALSE(lf::indirectly_unary_invocable<not_invocable, context_t, vec_iter>);
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_invocable<binary_fn, context_t, vec_iter>);
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_invocable<wrong_arg_fn, context_t, vec_iter>);
}

TEST_CASE("indirectly_unary_invocable: sync branch ignores Context", "[projected]") {
  // The sync side of the disjunction does not depend on Context, so a non-worker
  // context still validates a plain sync function but rejects an async-only one.
  STATIC_REQUIRE_FALSE(lf::worker_context<bad_ctx>);
  STATIC_REQUIRE(lf::indirectly_unary_invocable<sync_fn, bad_ctx, vec_iter>);
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_invocable<async_fn, bad_ctx, vec_iter>);
}

TEST_CASE("indirectly_unary_async_invocable: async-only variant", "[projected]") {
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_async_invocable<sync_fn, context_t, vec_iter>);
  STATIC_REQUIRE(lf::indirectly_unary_async_invocable<async_fn, context_t, vec_iter>);
  STATIC_REQUIRE(lf::indirectly_unary_async_invocable<hybrid_fn, context_t, vec_iter>);

  // copy_constructible<Fn> required by both branches.
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_async_invocable<async_fn_no_copy, context_t, vec_iter>);
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_invocable<async_fn_no_copy, context_t, vec_iter>);

  // indirectly_readable<I> required.
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_async_invocable<async_fn, context_t, int>);

  // worker_context<Context> required.
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_async_invocable<async_fn, bad_ctx, vec_iter>);
  STATIC_REQUIRE_FALSE(lf::indirectly_unary_async_invocable<hybrid_fn, bad_ctx, vec_iter>);
}

TEST_CASE("projected: pipelined / nested projection", "[projected]") {
  // Validates that `indirect_value`'s specialization for projected types kicks in;
  // otherwise the outer projection couldn't satisfy the concept.
  STATIC_REQUIRE(std::same_as<sync_then_sync::value_type, std::string>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<sync_then_sync>, std::string>);

  STATIC_REQUIRE(std::same_as<async_then_async::value_type, std::string>);
  STATIC_REQUIRE(std::same_as<std::iter_reference_t<async_then_async>, std::string>);

  STATIC_REQUIRE(std::same_as<sync_then_async::value_type, std::string>);
  STATIC_REQUIRE(std::same_as<async_then_sync::value_type, std::string>);
}
