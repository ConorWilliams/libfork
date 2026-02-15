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
  constexpr static auto release() noexcept -> void {}
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
  constexpr auto release() noexcept -> void {}
  constexpr auto acquire(std::byte *) noexcept -> void {}
};

static_assert(lf::stack_allocator<linear_allocator>);

template <lf::stack_allocator Alloc>
struct vector_ctx {

  using handle_type = lf::frame_handle<vector_ctx>;

  std::vector<handle_type> work;
  Alloc allocator;

  vector_ctx() { work.reserve(1024); }

  auto alloc() noexcept -> Alloc & { return allocator; }

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

  lf::deque<handle_type> work;
  Alloc allocator;

  auto alloc() noexcept -> Alloc & { return allocator; }

  // TODO: try LF_NO_INLINE for final allocator
  LF_NO_INLINE
  void push(handle_type handle) { work.push(handle); }

  auto pop() noexcept -> handle_type {
    return work.pop([] static -> handle_type {
      return {};
    });
  }
};

template <lf::stack_allocator Alloc>
struct poly_vector_ctx final : lf::polymorphic_context<Alloc> {

  using handle_type = lf::frame_handle<lf::polymorphic_context<Alloc>>;

  std::vector<handle_type> work;

  poly_vector_ctx() { work.reserve(1024); }

  void push(handle_type handle) override { work.push_back(handle); }

  auto pop() noexcept -> handle_type override {
    auto handle = work.back();
    work.pop_back();
    return handle;
  }
};

struct poly_deque_ctx final : lf::polymorphic_context<linear_allocator> {

  using handle_type = lf::frame_handle<lf::polymorphic_context<linear_allocator>>;

  lf::deque<handle_type> work;

  void push(handle_type handle) override { work.push(handle); }

  auto pop() noexcept -> handle_type override {
    return work.pop([] static -> handle_type {
      return {};
    });
  }
};

using lf::task;

template <lf::worker_context T>
constexpr auto no_await = [](this auto fib, std::int64_t *ret, std::int64_t n) -> task<void, T> {
  if (n < 2) {
    *ret = n;
    co_return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  auto t1 = fib(&lhs, n - 1);
  t1.promise->frame.kind = lf::category::root;
  t1.promise->frame.stack_ckpt = lf::thread_context<T>->alloc().checkpoint();
  t1.promise->frame.cancel = nullptr;
  t1.promise->handle().resume();

  auto t2 = fib(&rhs, n - 2);
  t2.promise->frame.kind = lf::category::root;
  t2.promise->frame.stack_ckpt = lf::thread_context<T>->alloc().checkpoint();
  t2.promise->frame.cancel = nullptr;
  t2.promise->handle().resume();

  *ret = lhs + rhs;
};

template <lf::worker_context T>
constexpr auto await = [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, T> {
  if (n < 2) {
    *ret = n;
    co_return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::call(fib, &lhs, n - 1);
  co_await lf::call(fib, &rhs, n - 2);

  *ret = lhs + rhs;
};

template <lf::worker_context T>
constexpr auto ret = [](this auto fib, std::int64_t n) -> lf::task<std::int64_t, T> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::call(&lhs, fib, n - 1);
  co_await lf::call(&rhs, fib, n - 2);

  co_return lhs + rhs;
};

template <typename T, bool Join = false>
constexpr auto fork_call = [](this auto fib, std::int64_t n) -> lf::task<std::int64_t, T> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::fork(&rhs, fib, n - 2);
  co_await lf::call(&lhs, fib, n - 1);

  if constexpr (Join) {
    co_await lf::join();
  }

  co_return lhs + rhs;
};

using global_alloc = vector_ctx<global_allocator>;
using linear_alloc = vector_ctx<linear_allocator>;

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

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;

    if constexpr (requires { Fn(&result, n); }) {
      auto task = Fn(&result, n);
      task.promise->frame.kind = lf::category::root;
      task.promise->frame.cancel = nullptr;
      task.promise->frame.stack_ckpt = lf::thread_context<U>->alloc().checkpoint();
      task.promise->handle().resume();
    } else {
      auto task = Fn(n);
      task.promise->frame.kind = lf::category::root;
      task.promise->frame.cancel = nullptr;
      task.promise->return_address = &result;
      task.promise->frame.stack_ckpt = lf::thread_context<U>->alloc().checkpoint();
      task.promise->handle().resume();
    }

    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

static_assert(lf::worker_context<global_alloc>);

// Return by ref-arg, test direct root, no co-await, direct resumes, uses new/delete for alloc
BENCHMARK(fib<no_await<global_alloc>, global_alloc>)->Name("test/libfork/fib/heap/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<global_alloc>, global_alloc>)->Name("base/libfork/fib/heap/no_await")->Arg(fib_base);

// Same as above but uses bump allocator
BENCHMARK(fib<no_await<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/bump/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/bump/no_await")->Arg(fib_base);

// TODO: no_await with segmented stack allocator?

// Return by ref-arg, libfork call/call with co-await, uses new/delete for alloc
BENCHMARK(fib<await<global_alloc>, global_alloc>)->Name("test/libfork/fib/heap/await")->Arg(fib_test);
BENCHMARK(fib<await<global_alloc>, global_alloc>)->Name("base/libfork/fib/heap/await")->Arg(fib_base);

// // Same as above but uses bump allocator
BENCHMARK(fib<await<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/bump/await")->Arg(fib_test);
BENCHMARK(fib<await<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/bump/await")->Arg(fib_base);

// Return by value
// libfork call/call with co-await
BENCHMARK(fib<ret<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/bump/return")->Arg(fib_test);
BENCHMARK(fib<ret<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/bump/return")->Arg(fib_base);

// Return by value
// libfork call/fork (no join)
// Non-polymorphic vector-backed context
BENCHMARK(fib<fork_call<linear_alloc>, linear_alloc>)->Name("test/libfork/fib/vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<linear_alloc>, linear_alloc>)->Name("base/libfork/fib/vector_ctx")->Arg(fib_base);

// Same as above but with join.
BENCHMARK(fib<fork_call<linear_alloc, true>, linear_alloc>)
    ->Name("test/libfork/fib/vector_ctx/join")
    ->Arg(fib_test);
BENCHMARK(fib<fork_call<linear_alloc, true>, linear_alloc>)
    ->Name("base/libfork/fib/vector_ctx/join")
    ->Arg(fib_base);

using A = poly_vector_ctx<linear_allocator>;
using B = lf::polymorphic_context<linear_allocator>;

// Same as above but with polymorphic contexts.
BENCHMARK(fib<fork_call<B>, A, B>)->Name("test/libfork/fib/poly_vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<B>, A, B>)->Name("base/libfork/fib/poly_vector_ctx")->Arg(fib_base);

// Same as above but with join.
BENCHMARK(fib<fork_call<B, true>, A, B>)->Name("test/libfork/fib/poly_vector_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<B, true>, A, B>)->Name("base/libfork/fib/poly_vector_ctx/join")->Arg(fib_base);

using C = poly_deque_ctx;
using D = deque_ctx<linear_allocator>;

// Return by value,
// Libfork call/join/fork with co-await,
// Deque-backed context
BENCHMARK(fib<fork_call<D, true>, D, D>)->Name("test/libfork/fib/deque_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<D, true>, D, D>)->Name("base/libfork/fib/deque_ctx/join")->Arg(fib_base);

// Same as above but polymorphic
BENCHMARK(fib<fork_call<B, true>, C, B>)->Name("test/libfork/fib/poly_deque_ctx/join")->Arg(fib_test);
BENCHMARK(fib<fork_call<B, true>, C, B>)->Name("base/libfork/fib/poly_deque_ctx/join")->Arg(fib_base);
