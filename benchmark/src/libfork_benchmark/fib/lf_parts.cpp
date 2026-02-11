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
  t1.promise->handle().resume();

  auto t2 = fib(&rhs, n - 2);
  t2.promise->frame.kind = lf::category::root;
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

template <typename T>
constexpr auto fork_call = [](this auto fib, std::int64_t n) -> lf::task<std::int64_t, T> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::fork(&rhs, fib, n - 2);
  co_await lf::call(&lhs, fib, n - 1);

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
      task.promise->handle().resume();
    } else {
      auto task = Fn(n);
      task.promise->frame.kind = lf::category::root;
      task.promise->return_address = &result;
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

using A = poly_vector_ctx<linear_allocator>;
using B = lf::polymorphic_context<linear_allocator>;

// Same as above but with polymorphic contexts.
BENCHMARK(fib<fork_call<B>, A, B>)->Name("test/libfork/fib/poly_vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<B>, A, B>)->Name("base/libfork/fib/poly_vector_ctx")->Arg(fib_base);
