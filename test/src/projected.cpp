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

// ============================================================================
// `projected`: shape, value_type, dereference, iter_*_t
// ============================================================================

using sync_projected = lf::projected<vec_iter, sync_proj, context_t>;
using async_projected = lf::projected<vec_iter, async_proj, context_t>;
using hybrid_projected = lf::projected<vec_iter, hybrid_proj, context_t>;

static_assert(std::same_as<sync_projected::value_type, double>);
static_assert(std::same_as<async_projected::value_type, double>);
static_assert(std::same_as<hybrid_projected::value_type, double>);

static_assert(std::same_as<sync_projected::difference_type, std::iter_difference_t<vec_iter>>);
static_assert(std::same_as<async_projected::difference_type, std::iter_difference_t<vec_iter>>);
static_assert(std::same_as<hybrid_projected::difference_type, std::iter_difference_t<vec_iter>>);

static_assert(std::indirectly_readable<sync_projected>);
static_assert(std::indirectly_readable<async_projected>);
static_assert(std::indirectly_readable<hybrid_projected>);

// dereference returns the (possibly-ref) result of the projection.
static_assert(std::same_as<std::iter_reference_t<sync_projected>, double>);
static_assert(std::same_as<std::iter_reference_t<async_projected>, double>);
static_assert(std::same_as<std::iter_reference_t<hybrid_projected>, double>);

static_assert(std::same_as<std::iter_value_t<sync_projected>, double>);
static_assert(std::same_as<std::iter_value_t<async_projected>, double>);

// const-iterator source.
using sync_cprojected = lf::projected<cvec_iter, sync_proj, context_t>;
static_assert(std::same_as<sync_cprojected::value_type, double>);
static_assert(std::same_as<std::iter_reference_t<sync_cprojected>, double>);

// Reference-returning projection: value_type is the underlying value, deref is the ref.
using sync_ref_projected = lf::projected<vec_iter, sync_ref_proj, context_t>;
static_assert(std::same_as<sync_ref_projected::value_type, int>);
static_assert(std::same_as<std::iter_reference_t<sync_ref_projected>, int &>);

// ============================================================================
// `difference_type` only present when the source is `weakly_incrementable`.
// ============================================================================

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
static_assert(has_difference_type<async_projected>);

// ============================================================================
// `indirectly_unary_invocable` — accepts sync, async, or both.
// ============================================================================

static_assert(lf::indirectly_unary_invocable<sync_fn, context_t, vec_iter>);
static_assert(lf::indirectly_unary_invocable<async_fn, context_t, vec_iter>);
static_assert(lf::indirectly_unary_invocable<hybrid_fn, context_t, vec_iter>);

static_assert(!lf::indirectly_unary_invocable<not_invocable, context_t, vec_iter>);
static_assert(!lf::indirectly_unary_invocable<binary_fn, context_t, vec_iter>);
static_assert(!lf::indirectly_unary_invocable<wrong_arg_fn, context_t, vec_iter>);

// The sync branch of the disjunction does not depend on Context — a non-worker
// Context still validates a plain sync function.
struct bad_ctx {};
static_assert(!lf::worker_context<bad_ctx>);
static_assert(lf::indirectly_unary_invocable<sync_fn, bad_ctx, vec_iter>);
static_assert(!lf::indirectly_unary_invocable<async_fn, bad_ctx, vec_iter>);

// ============================================================================
// `indirectly_unary_async_invocable` — async-only variant.
// ============================================================================

static_assert(!lf::indirectly_unary_async_invocable<sync_fn, context_t, vec_iter>);
static_assert(lf::indirectly_unary_async_invocable<async_fn, context_t, vec_iter>);
static_assert(lf::indirectly_unary_async_invocable<hybrid_fn, context_t, vec_iter>);

// `copy_constructible<Fn>` is required by both branches.
static_assert(!lf::indirectly_unary_async_invocable<async_fn_no_copy, context_t, vec_iter>);
static_assert(!lf::indirectly_unary_invocable<async_fn_no_copy, context_t, vec_iter>);

// `indirectly_readable<I>` is required.
static_assert(!lf::indirectly_unary_async_invocable<async_fn, context_t, int>);

// Non-worker context never satisfies the async variant.
static_assert(!lf::indirectly_unary_async_invocable<async_fn, bad_ctx, vec_iter>);
static_assert(!lf::indirectly_unary_async_invocable<hybrid_fn, bad_ctx, vec_iter>);

// ============================================================================
// Pipelining: `projected` of a `projected`.
// Validates that `indirect_value`'s specialization for projected types kicks in
// (otherwise the outer projection couldn't satisfy the concept).
// ============================================================================

struct str_proj {
  auto operator()(double const &) const -> std::string;
};

struct async_str_proj {
  template <typename Context>
  static auto operator()(lf::env<Context>, double const &) -> lf::task<std::string, Context>;
};

using sync_then_sync = lf::projected<sync_projected, str_proj, context_t>;
static_assert(std::same_as<sync_then_sync::value_type, std::string>);
static_assert(std::same_as<std::iter_reference_t<sync_then_sync>, std::string>);

using async_then_async = lf::projected<async_projected, async_str_proj, context_t>;
static_assert(std::same_as<async_then_async::value_type, std::string>);
static_assert(std::same_as<std::iter_reference_t<async_then_async>, std::string>);

using sync_then_async = lf::projected<sync_projected, async_str_proj, context_t>;
static_assert(std::same_as<sync_then_async::value_type, std::string>);

using async_then_sync = lf::projected<async_projected, str_proj, context_t>;
static_assert(std::same_as<async_then_sync::value_type, std::string>);

} // namespace

TEST_CASE("projected: compile-only checks", "[projected]") {
  SUCCEED("static_asserts above");
}
