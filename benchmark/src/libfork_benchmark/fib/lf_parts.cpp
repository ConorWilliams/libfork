#include <coroutine>
#include <cstddef>
#include <new>

#include <benchmark/benchmark.h>

#include "libfork/__impl/assume.hpp"

#include "libfork_benchmark/fib/fib.hpp"

import libfork.core;

namespace {

struct global_allocator {

  struct empty {
    auto operator==(empty const &) const -> bool = default;
  };

  constexpr static auto push(std::size_t sz) -> void * { return ::operator new(sz); }
  constexpr static auto pop(void *p, std::size_t sz) noexcept -> void { ::operator delete(p, sz); }
  constexpr static auto checkpoint() noexcept -> empty { return {}; }
  constexpr static auto release(empty) noexcept -> void {}
  constexpr static auto prepare_release() noexcept -> empty { return {}; }
  constexpr static auto acquire(empty) noexcept -> void {}
};

static_assert(lf::stack_allocator<global_allocator>);

struct linear_allocator {

  std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(1024 * 1024);
  std::byte *ptr = data.get();

  constexpr auto push(std::size_t sz) -> void * {
    auto *prev = ptr;
    ptr += fib_align_size(sz);
    return prev;
  }
  constexpr auto pop(void *p, std::size_t) noexcept -> void { ptr = static_cast<std::byte *>(p); }

  constexpr auto checkpoint() noexcept -> std::byte * { return data.get(); }
  constexpr auto prepare_release() noexcept -> std::byte * { return data.get(); }
  constexpr auto release(std::byte *) noexcept -> void {}
  constexpr auto acquire(std::byte *) noexcept -> void {}
};

static_assert(lf::stack_allocator<linear_allocator>);

template <lf::stack_allocator Alloc>
struct vector_ctx {

  using handle_type = lf::frame_handle<vector_ctx>;

  std::vector<handle_type> work;
  Alloc my_allocator;

  vector_ctx() { work.reserve(1024); }

  auto allocator() noexcept -> Alloc & { return my_allocator; }

  void post(lf::await_handle<vector_ctx>) {}

  // TODO: try LF_NO_INLINE for final allocator
  LF_NO_INLINE
  void push(handle_type handle) { work.push_back(handle); }

  auto pop() noexcept -> handle_type {
    auto handle = work.back();
    work.pop_back();
    return handle;
  }
};

template <lf::stack_allocator Alloc>
struct deque_ctx {

  using handle_type = lf::frame_handle<deque_ctx>;

  lf::deque<handle_type> work{64};
  Alloc my_allocator;

  auto allocator() noexcept -> Alloc & { return my_allocator; }

  void post(lf::await_handle<deque_ctx>) {}

  // TODO: try LF_NO_INLINE for final allocator
  void push(handle_type handle) { work.push(handle); }

  auto pop() noexcept -> handle_type {
    return work.pop([] static -> handle_type {
      return {};
    });
  }
};

template <lf::stack_allocator Alloc>
struct poly_vector_ctx final : lf::basic_poly_context<Alloc> {

  using handle_type = lf::frame_handle<lf::basic_poly_context<Alloc>>;

  std::vector<handle_type> work;

  poly_vector_ctx() { work.reserve(1024); }

  void post(lf::await_handle<lf::basic_poly_context<Alloc>>) override {}

  void push(handle_type handle) override { work.push_back(handle); }

  auto pop() noexcept -> handle_type override {
    auto handle = work.back();
    work.pop_back();
    return handle;
  }
};

template <lf::stack_allocator Alloc>
struct poly_deque_ctx final : lf::basic_poly_context<Alloc> {

  using handle_type = lf::frame_handle<lf::basic_poly_context<Alloc>>;

  lf::deque<handle_type> work{64};

  void post(lf::await_handle<lf::basic_poly_context<Alloc>>) override {}

  void push(handle_type handle) override { work.push(handle); }

  auto pop() noexcept -> handle_type override {
    return work.pop([] static -> handle_type {
      return {};
    });
  }
};

using lf::task;

template <lf::worker_context T>
constexpr auto await = [](this auto fib, lf::env<T>, std::int64_t *ret, std::int64_t n) -> lf::task<void> {
  if (n < 2) {
    *ret = n;
    co_return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  using scope = lf::scope<T>;

  co_await scope::call(fib, &lhs, n - 1);
  co_await scope::call(fib, &rhs, n - 2);

  *ret = lhs + rhs;
};

template <lf::worker_context T>
constexpr auto ret = [](this auto fib, lf::env<T>, std::int64_t n) -> lf::task<std::int64_t> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  using scope = lf::scope<T>;

  co_await scope::call(&lhs, fib, n - 1);
  co_await scope::call(&rhs, fib, n - 2);

  co_return lhs + rhs;
};

template <lf::worker_context T, bool Join = false>
constexpr auto fork_call = [](this auto fib, lf::env<T>, std::int64_t n) -> lf::task<std::int64_t> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  using scope = lf::scope<T>;

  co_await scope::fork(&rhs, fib, n - 2);
  co_await scope::call(&lhs, fib, n - 1);

  if constexpr (Join) {
    co_await lf::join();
  }

  co_return lhs + rhs;
};

using global_alloc = vector_ctx<global_allocator>;
using linear_alloc = vector_ctx<linear_allocator>;
using stacks_alloc = vector_ctx<lf::geometric_stack>;

template <auto Fn, typename T, typename U = T>
void fib(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  T context;

  lf::thread_context<U> = static_cast<U *>(&context);

  lf::defer _ = [] static noexcept {
    lf::thread_context<U> = nullptr;
  };

  U *launch = &context;

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);

    std::int64_t return_value = 0;

    if constexpr (requires { lf::schedule(launch, Fn, &return_value, n); }) {
      lf::schedule(launch, Fn, &return_value, n);
    } else {
      return_value = lf::schedule(launch, Fn, n);
    }

    CHECK_RESULT(return_value, expect);
    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

static_assert(lf::worker_context<global_alloc>);
static_assert(lf::worker_context<linear_alloc>);
static_assert(lf::worker_context<stacks_alloc>);

// Return by ref-arg, libfork call/call with co-await, uses new/delete for alloc
BENCHMARK(fib<await<global_alloc>, global_alloc>)->Name("test/libfork/fib/heap/await")->Arg(fib_test);
BENCHMARK(fib<await<global_alloc>, global_alloc>)->Name("base/libfork/fib/heap/await")->Arg(fib_base);

// Same as above but uses bump allocator
BENCHMARK(fib<await<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/bump/await")->Arg(fib_test);
BENCHMARK(fib<await<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/bump/await")->Arg(fib_base);

// Same as above but uses geometric stack
BENCHMARK(fib<await<stacks_alloc>, stacks_alloc>)->Name("test/libfork/fib/geometric/await")->Arg(fib_test);
BENCHMARK(fib<await<stacks_alloc>, stacks_alloc>)->Name("base/libfork/fib/geometric/await")->Arg(fib_base);

// Return by value
// libfork call/call with co-await
BENCHMARK(fib<ret<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/bump/return")->Arg(fib_test);
BENCHMARK(fib<ret<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/bump/return")->Arg(fib_base);

BENCHMARK(fib<ret<stacks_alloc>, stacks_alloc>)->Name("test/libfork/fib/geometric/return")->Arg(fib_test);
BENCHMARK(fib<ret<stacks_alloc>, stacks_alloc>)->Name("base/libfork/fib/geometric/return")->Arg(fib_base);

// Return by value
// libfork call/fork (no join)
// Non-polymorphic vector-backed context
BENCHMARK(fib<fork_call<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/vector_ctx")->Arg(fib_base);

BENCHMARK(fib<fork_call<stacks_alloc>, stacks_alloc>)
    ->Name("test/libfork/fib/geometric/vector_ctx")
    ->Arg(fib_test);
BENCHMARK(fib<fork_call<stacks_alloc>, stacks_alloc>)
    ->Name("base/libfork/fib/geometric/vector_ctx")
    ->Arg(fib_base);

using A = poly_vector_ctx<linear_allocator>;
using B = lf::basic_poly_context<linear_allocator>;

// Same as above but with polymorphic contexts.
BENCHMARK(fib<fork_call<B>, A, B>)->Name("test/libfork/fib/poly_vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<B>, A, B>)->Name("base/libfork/fib/poly_vector_ctx")->Arg(fib_base);

using E = poly_vector_ctx<lf::geometric_stack>;
using F = lf::basic_poly_context<lf::geometric_stack>;

BENCHMARK(fib<fork_call<F>, E, F>)->Name("test/libfork/fib/poly_geometric_vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<F>, E, F>)->Name("base/libfork/fib/poly_geometric_vector_ctx")->Arg(fib_base);

// Same as above but with join and non-polymorphic.
BENCHMARK(fib<fork_call<linear_alloc, true>, linear_alloc>)
    ->Name("test/libfork/fib/vector_ctx/join")
    ->Arg(fib_test);
BENCHMARK(fib<fork_call<linear_alloc, true>, linear_alloc>)
    ->Name("base/libfork/fib/vector_ctx/join")
    ->Arg(fib_base);

BENCHMARK(fib<fork_call<stacks_alloc, true>, stacks_alloc>)
    ->Name("test/libfork/fib/geometric/vector_ctx/join")
    ->Arg(fib_test);
BENCHMARK(fib<fork_call<stacks_alloc, true>, stacks_alloc>)
    ->Name("base/libfork/fib/geometric/vector_ctx/join")
    ->Arg(fib_base);

// Same as above but with join and polymorphic.
BENCHMARK(fib<fork_call<B, true>, A, B>)->Name("test/libfork/fib/poly_vector_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<B, true>, A, B>)->Name("base/libfork/fib/poly_vector_ctx/join")->Arg(fib_base);

BENCHMARK(fib<fork_call<F, true>, E, F>)
    ->Name("test/libfork/fib/poly_geometric_vector_ctx/join")
    ->Arg(fib_test);
BENCHMARK(fib<fork_call<F, true>, E, F>)
    ->Name("base/libfork/fib/poly_geometric_vector_ctx/join")
    ->Arg(fib_base);

using C = poly_deque_ctx<linear_allocator>;
using D = deque_ctx<linear_allocator>;
using G = deque_ctx<lf::geometric_stack>;
using H = poly_deque_ctx<lf::geometric_stack>;

// Return by value,
// Libfork call/join/fork with co-await,
// Deque-backed context
BENCHMARK(fib<fork_call<D, true>, D, D>)->Name("test/libfork/fib/deque_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<D, true>, D, D>)->Name("base/libfork/fib/deque_ctx/join")->Arg(fib_base);

BENCHMARK(fib<fork_call<G, true>, G, G>)->Name("test/libfork/fib/geometric/deque_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<G, true>, G, G>)->Name("base/libfork/fib/geometric/deque_ctx/join")->Arg(fib_base);

// Same as above but polymorphic
BENCHMARK(fib<fork_call<B, true>, C, B>)->Name("test/libfork/fib/poly_deque_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<B, true>, C, B>)->Name("base/libfork/fib/poly_deque_ctx/join")->Arg(fib_base);

BENCHMARK(fib<fork_call<F, true>, H, F>)
    ->Name("test/libfork/fib/poly_geometric_deque_ctx/join")
    ->Arg(fib_test);
BENCHMARK(fib<fork_call<F, true>, H, F>)
    ->Name("base/libfork/fib/poly_geometric_deque_ctx/join")
    ->Arg(fib_base);
