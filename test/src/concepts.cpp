#include <catch2/catch_test_macros.hpp>

import std;

import libfork;
import libfork.utils;

using namespace lf;

using test_stack = geometric_stack<>;
using test_context = mono_context<test_stack, adapt_vector<>>;

TEST_CASE("Concepts: atomicable", "[concepts]") {
  STATIC_REQUIRE(atomicable<std::byte>);
  STATIC_REQUIRE(atomicable<void *>);

  struct trivial {
    int x;
    float y;
  };

  STATIC_REQUIRE(atomicable<trivial>);

  STATIC_REQUIRE_FALSE(atomicable<std::string>);
  STATIC_REQUIRE_FALSE(atomicable<const int>);
  STATIC_REQUIRE_FALSE(atomicable<int &>);
}

TEST_CASE("Concepts: lock_free", "[concepts]") {
  STATIC_REQUIRE(lock_free<std::byte>);
  STATIC_REQUIRE(lock_free<void *>);
}

namespace {

template <typename... T>
struct my_template {};

} // namespace

TEST_CASE("Concepts: specialization_of", "[concepts]") {
  STATIC_REQUIRE(specialization_of<std::vector<int>, std::vector>);
  STATIC_REQUIRE(specialization_of<my_template<int, float>, my_template>);
  STATIC_REQUIRE(specialization_of<task<int, test_context>, task>);

  STATIC_REQUIRE_FALSE(specialization_of<int, std::vector>);
  STATIC_REQUIRE_FALSE(specialization_of<std::vector<int>, my_template>);
}

TEST_CASE("Concepts: returnable", "[concepts]") {
  STATIC_REQUIRE(returnable<void>);
  STATIC_REQUIRE(returnable<int>);
  STATIC_REQUIRE(returnable<std::unique_ptr<int>>);

  struct non_movable {
    non_movable() = default;
    non_movable(const non_movable &) = delete;
    non_movable(non_movable &&) = delete;
  };

  STATIC_REQUIRE_FALSE(returnable<non_movable>);
}

TEST_CASE("Concepts: worker_stack", "[concepts]") {
  STATIC_REQUIRE(worker_stack<test_stack>);

  struct bad_stack {
    struct ckpt {
      auto operator==(ckpt const &) const -> bool = default;
    };
    static auto push(std::size_t) -> void *;
    static auto pop(void *, std::size_t) -> void; // missing noexcept
    static auto checkpoint() noexcept -> ckpt;
    static auto prepare_release() noexcept -> int;
    static auto release(int) noexcept -> void;
    static auto acquire(ckpt const &) noexcept -> void;
  };

  STATIC_REQUIRE_FALSE(worker_stack<bad_stack>);
}

TEST_CASE("Concepts: worker_context", "[concepts]") {
  STATIC_REQUIRE(worker_context<test_context>);

  struct missing_push {
    auto pop() noexcept -> lf::steal_handle<missing_push>;
    auto stack() noexcept -> test_stack &;
  };

  STATIC_REQUIRE_FALSE(worker_context<missing_push>);
}

TEST_CASE("Concepts: async_invocable", "[concepts]") {

  auto async_fn_env(env<test_context>, int) -> task<int, test_context>;
  auto async_fn_no_env(int) -> task<int, test_context>;
  auto not_async_fn(int) -> int;

  struct both_invocable {
    auto operator()(env<test_context>, int) const -> task<int, test_context>;
    auto operator()(int) const -> task<double, test_context>;
  };

  // Basic positive cases
  STATIC_REQUIRE(async_invocable<decltype(async_fn_env), test_context, int>);
  STATIC_REQUIRE(async_invocable<decltype(async_fn_no_env), test_context, int>);

  // Arg mismatch
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_env), test_context, int *>);
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_no_env), test_context, double *>);

  // Result type check
  STATIC_REQUIRE(std::same_as<async_result_t<decltype(async_fn_env), test_context, int>, int>);

  // Preference check: when both are available, it should pick the one with env
  // and return int task.
  STATIC_REQUIRE(async_invocable_to<both_invocable, int, test_context, int>);
  // Verification that it didn't pick the double one
  STATIC_REQUIRE_FALSE(async_invocable_to<both_invocable, double, test_context, int>);

  // Fails unless return is a task
  STATIC_REQUIRE_FALSE(async_invocable<decltype(not_async_fn), test_context, int>);

  // Need a valid context of a different type
  struct mock_context {
    void push(lf::steal_handle<mock_context>);
    void post(lf::sched_handle<mock_context>);
    auto pop() noexcept -> lf::steal_handle<mock_context>;
    auto stack() noexcept -> test_stack &;
  };

  STATIC_REQUIRE(worker_context<mock_context>);

  // Fails because the result task's context doesn't match the provided context
  STATIC_REQUIRE_FALSE(async_invocable<decltype(async_fn_no_env), mock_context, int>);
}

TEST_CASE("Concepts: async_nothrow_invocable", "[concepts]") {

  struct nothrow_async {
    auto operator()(int) const noexcept -> task<int, test_context>;
  };

  struct throwing_async {
    auto operator()(int) const -> task<int, test_context>;
  };

  STATIC_REQUIRE(async_nothrow_invocable<nothrow_async, test_context, int>);
  STATIC_REQUIRE_FALSE(async_nothrow_invocable<throwing_async, test_context, int>);
}

namespace {

using semigroup_iter = std::vector<int>::iterator;
using semigroup_const_iter = std::vector<int>::const_iterator;

struct semigroup_context {
  void push(lf::steal_handle<semigroup_context>);
  void post(lf::sched_handle<semigroup_context>);
  auto pop() noexcept -> lf::steal_handle<semigroup_context>;
  auto stack() noexcept -> test_stack &;
};

static_assert(worker_context<semigroup_context>);

struct semigroup_bad_context {};

struct semigroup_acc {
  semigroup_acc() = default;
  explicit semigroup_acc(int);
};

struct semigroup_non_default {
  semigroup_non_default() = delete;
  explicit semigroup_non_default(int);
  semigroup_non_default(semigroup_non_default const &) = default;
  semigroup_non_default(semigroup_non_default &&) = default;
  auto operator=(semigroup_non_default const &) -> semigroup_non_default & = default;
  auto operator=(semigroup_non_default &&) -> semigroup_non_default & = default;
};

static_assert(returnable<semigroup_non_default>);
static_assert(!std::default_initializable<semigroup_non_default>);

struct sync_int_semigroup {
  auto operator()(int, int) const -> int;
};

struct sync_acc_semigroup {
  auto operator()(int, int) const -> semigroup_acc;
  auto operator()(int, semigroup_acc) const -> semigroup_acc;
  auto operator()(semigroup_acc, int) const -> semigroup_acc;
  auto operator()(semigroup_acc, semigroup_acc) const -> semigroup_acc;
};

struct sync_mutable_ref_semigroup {
  auto operator()(int &, int &) const -> semigroup_acc;
  auto operator()(int &, semigroup_acc) const -> semigroup_acc;
  auto operator()(semigroup_acc, int &) const -> semigroup_acc;
  auto operator()(semigroup_acc, semigroup_acc) const -> semigroup_acc;
};

struct sync_missing_mixed {
  auto operator()(int, int) const -> semigroup_acc;
  auto operator()(semigroup_acc, semigroup_acc) const -> semigroup_acc;
};

struct sync_wrong_mixed_return {
  auto operator()(int, int) const -> semigroup_acc;
  auto operator()(int, semigroup_acc) const -> int;
  auto operator()(semigroup_acc, int) const -> semigroup_acc;
  auto operator()(semigroup_acc, semigroup_acc) const -> semigroup_acc;
};

struct sync_not_copyable {
  sync_not_copyable() = default;
  sync_not_copyable(sync_not_copyable const &) = delete;
  sync_not_copyable(sync_not_copyable &&) = default;
  auto operator()(int, int) const -> int;
};

struct sync_project_int {
  auto operator()(int const &) const -> int;
};

using sync_projected_iter = projected<test_context, semigroup_iter, sync_project_int>;

struct async_project_int {
  template <typename Context>
  static auto operator()(env<Context>, int const &) -> task<int, Context>;
};

using async_projected_iter = projected<test_context, semigroup_iter, async_project_int>;

struct async_int_semigroup {
  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<int, Context>;
};

struct async_acc_semigroup {
  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<semigroup_acc, Context>;
  template <typename Context>
  static auto operator()(env<Context>, int, semigroup_acc) -> task<semigroup_acc, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_acc, int) -> task<semigroup_acc, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_acc, semigroup_acc) -> task<semigroup_acc, Context>;
};

struct async_missing_mixed {
  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<semigroup_acc, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_acc, semigroup_acc) -> task<semigroup_acc, Context>;
};

struct async_wrong_mixed_return {
  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<semigroup_acc, Context>;
  template <typename Context>
  static auto operator()(env<Context>, int, semigroup_acc) -> task<int, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_acc, int) -> task<semigroup_acc, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_acc, semigroup_acc) -> task<semigroup_acc, Context>;
};

struct async_wrong_context {
  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<int, semigroup_context>;
};

struct async_non_default_result {
  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<semigroup_non_default, Context>;
  template <typename Context>
  static auto operator()(env<Context>, int, semigroup_non_default) -> task<semigroup_non_default, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_non_default, int) -> task<semigroup_non_default, Context>;
  template <typename Context>
  static auto operator()(env<Context>, semigroup_non_default, semigroup_non_default)
      -> task<semigroup_non_default, Context>;
};

struct async_not_copyable {
  async_not_copyable() = default;
  async_not_copyable(async_not_copyable const &) = delete;
  async_not_copyable(async_not_copyable &&) = default;

  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<int, Context>;
};

struct hybrid_semigroup {
  auto operator()(int, int) const -> long;

  template <typename Context>
  static auto operator()(env<Context>, int, int) -> task<double, Context>;
};

} // namespace

TEST_CASE("Concepts: sync indirect_semigroup", "[concepts]") {
  STATIC_REQUIRE(lf::sync::indirect_semigroup<sync_int_semigroup, semigroup_iter>);
  STATIC_REQUIRE(lf::sync::indirect_semigroup<sync_int_semigroup, semigroup_const_iter>);
  STATIC_REQUIRE(lf::sync::indirect_semigroup<sync_acc_semigroup, semigroup_const_iter>);
  STATIC_REQUIRE(lf::sync::indirect_semigroup<sync_int_semigroup, sync_projected_iter>);
  STATIC_REQUIRE(lf::sync::indirect_semigroup<sync_int_semigroup, async_projected_iter>);

  // A mutable-reference-only callable works for mutable iterators where
  // indirect_value_t and iter_reference_t are both int&.
  STATIC_REQUIRE(lf::sync::indirect_semigroup<sync_mutable_ref_semigroup, semigroup_iter>);

  // const_iterator has indirect_value_t<int&> but iter_reference_t<const int&>,
  // so the iter_reference_t checks reject mutable-only binary operations.
  STATIC_REQUIRE_FALSE(lf::sync::indirect_semigroup<sync_mutable_ref_semigroup, semigroup_const_iter>);

  STATIC_REQUIRE_FALSE(lf::sync::indirect_semigroup<sync_missing_mixed, semigroup_const_iter>);
  STATIC_REQUIRE_FALSE(lf::sync::indirect_semigroup<sync_wrong_mixed_return, semigroup_const_iter>);
  STATIC_REQUIRE_FALSE(lf::sync::indirect_semigroup<sync_not_copyable, semigroup_iter>);
  STATIC_REQUIRE_FALSE(lf::sync::indirect_semigroup<sync_int_semigroup, int>);
  STATIC_REQUIRE_FALSE(lf::sync::indirect_semigroup<async_int_semigroup, semigroup_iter>);
}

TEST_CASE("Concepts: async indirect_semigroup", "[concepts]") {
  STATIC_REQUIRE(lf::async::indirect_semigroup<async_int_semigroup, test_context, semigroup_iter>);
  STATIC_REQUIRE(lf::async::indirect_semigroup<async_int_semigroup, test_context, semigroup_const_iter>);
  STATIC_REQUIRE(lf::async::indirect_semigroup<async_acc_semigroup, test_context, semigroup_const_iter>);
  STATIC_REQUIRE(lf::async::indirect_semigroup<async_int_semigroup, test_context, sync_projected_iter>);
  STATIC_REQUIRE(lf::async::indirect_semigroup<async_int_semigroup, test_context, async_projected_iter>);

  STATIC_REQUIRE_FALSE(lf::async::indirect_semigroup<sync_int_semigroup, test_context, semigroup_iter>);
  STATIC_REQUIRE_FALSE(
      lf::async::indirect_semigroup<async_missing_mixed, test_context, semigroup_const_iter>);
  STATIC_REQUIRE_FALSE(
      lf::async::indirect_semigroup<async_wrong_mixed_return, test_context, semigroup_const_iter>);
  STATIC_REQUIRE_FALSE(lf::async::indirect_semigroup<async_wrong_context, test_context, semigroup_iter>);
  STATIC_REQUIRE_FALSE(lf::async::indirect_semigroup<async_non_default_result, test_context, semigroup_iter>);
  STATIC_REQUIRE_FALSE(lf::async::indirect_semigroup<async_not_copyable, test_context, semigroup_iter>);
  STATIC_REQUIRE_FALSE(
      lf::async::indirect_semigroup<async_int_semigroup, semigroup_bad_context, semigroup_iter>);
  STATIC_REQUIRE_FALSE(lf::async::indirect_semigroup<async_int_semigroup, test_context, int>);
}

TEST_CASE("Concepts: indirect_semigroup dispatch and result type", "[concepts]") {
  STATIC_REQUIRE(indirect_semigroup<sync_int_semigroup, semigroup_bad_context, semigroup_iter>);
  STATIC_REQUIRE(indirect_semigroup<async_int_semigroup, test_context, semigroup_iter>);
  STATIC_REQUIRE(indirect_semigroup<hybrid_semigroup, test_context, semigroup_iter>);

  STATIC_REQUIRE_FALSE(indirect_semigroup<async_int_semigroup, semigroup_bad_context, semigroup_iter>);
  STATIC_REQUIRE_FALSE(indirect_semigroup<sync_missing_mixed, test_context, semigroup_const_iter>);

  STATIC_REQUIRE(
      std::same_as<indirect_semigroup_t<sync_int_semigroup, semigroup_bad_context, semigroup_iter>, int>);
  STATIC_REQUIRE(std::same_as<indirect_semigroup_t<sync_acc_semigroup, semigroup_bad_context, semigroup_iter>,
                              semigroup_acc>);
  STATIC_REQUIRE(std::same_as<indirect_semigroup_t<async_int_semigroup, test_context, semigroup_iter>, int>);

  // When both branches match, the async-specialized result type wins.
  STATIC_REQUIRE(std::same_as<indirect_semigroup_t<hybrid_semigroup, test_context, semigroup_iter>, double>);
}

namespace {

struct plain_awaitable {};

struct member_co_await {
  auto operator co_await() -> plain_awaitable;
};

struct free_co_await {};

[[maybe_unused]]
auto operator co_await(free_co_await) -> plain_awaitable &;

struct both_co_await {
  auto operator co_await() -> plain_awaitable;
};

[[maybe_unused]]
auto operator co_await(both_co_await) -> plain_awaitable;

template <typename T>
concept can_acquire = requires (T &&t) { acquire_awaitable(std::forward<T>(t)); };

} // namespace

TEST_CASE("Concepts: awaitable_acquirable", "[concepts]") {
  // Generic identity overload accepts any type — even non-awaiters.
  STATIC_REQUIRE(can_acquire<plain_awaitable>);
  STATIC_REQUIRE(can_acquire<int>);

  // A single operator co_await — member or free — disambiguates the dispatch.
  STATIC_REQUIRE(can_acquire<member_co_await>);
  STATIC_REQUIRE(can_acquire<free_co_await>);

  // Defining BOTH member and free operator co_await leaves the dispatch ambiguous.
  STATIC_REQUIRE_FALSE(can_acquire<both_co_await>);
}

TEST_CASE("acquire_awaitable", "[concepts]") {
  // No operator co_await: returns the argument unchanged, preserving value category.
  using acq_plain_xref = decltype(acquire_awaitable(std::declval<plain_awaitable>()));
  using acq_plain_lref = decltype(acquire_awaitable(std::declval<plain_awaitable &>()));
  using acq_plain_cref = decltype(acquire_awaitable(std::declval<plain_awaitable const &>()));

  STATIC_REQUIRE(std::same_as<acq_plain_xref, plain_awaitable &&>);
  STATIC_REQUIRE(std::same_as<acq_plain_lref, plain_awaitable &>);
  STATIC_REQUIRE(std::same_as<acq_plain_cref, plain_awaitable const &>);

  // Member operator co_await: returns whatever the member call produces.
  STATIC_REQUIRE(std::same_as<decltype(acquire_awaitable(std::declval<member_co_await>())), plain_awaitable>);

  // Free operator co_await: returns whatever the ADL-found free call produces.
  STATIC_REQUIRE(std::same_as<decltype(acquire_awaitable(std::declval<free_co_await>())), plain_awaitable &>);
}

namespace {

struct lf_awaitable {
  auto await_ready() -> bool;
  auto await_suspend(lf::sched_handle<test_context>, test_context &) -> void;
  auto await_resume() -> void;
};

struct bad_lf_awaitable {
  auto await_ready() -> bool;
  auto await_suspend(lf::sched_handle<test_context>, int &) -> void; // Wrong context type
  auto await_resume() -> void;
};

} // namespace

TEST_CASE("Concepts: awaitable_impl", "[concepts]") {

  STATIC_REQUIRE(lf::awaitable<lf_awaitable, test_context>);

  STATIC_REQUIRE_FALSE(lf::awaitable<bad_lf_awaitable, test_context>);
}
