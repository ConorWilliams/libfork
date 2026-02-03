#include <coroutine>
#include <cstddef>
#include <new>

#include <benchmark/benchmark.h>

#include "libfork/__impl/assume.hpp"

#include "libfork_benchmark/fib/fib.hpp"

import libfork.core;

namespace {

struct stack_on_heap {
  static constexpr auto operator new(std::size_t sz) -> void * { return ::operator new(sz); }
  static constexpr auto operator delete(void *p, [[maybe_unused]] std::size_t sz) noexcept -> void {
    ::operator delete(p, sz);
  }
};

template <lf::alloc_mixin Stack>
constexpr auto no_await = [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, Stack> {
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

template <lf::alloc_mixin Stack>
constexpr auto await = [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, Stack> {
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

constexpr auto ret = [](this auto fib, std::int64_t n) -> lf::task<std::int64_t, tls_bump> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::call(&lhs, fib, n - 1);
  co_await lf::call(&rhs, fib, n - 2);

  co_return lhs + rhs;
};

template <typename Ctx, typename A = tls_bump>
constexpr auto fork_call = [](this auto fib, std::int64_t n) -> lf::task<std::int64_t, A, Ctx> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::fork(&rhs, fib, n - 2);
  co_await lf::call(&lhs, fib, n - 1);

  co_return lhs + rhs;
};

template <auto Fn>
void fib(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  // Set bump allocator buffer
  std::unique_ptr buf = std::make_unique<std::byte[]>(1024 * 1024);
  tls_bump_ptr = buf.get();

  // Set both context and poly context
  std::unique_ptr ctx = std::make_unique<vector_ctx>();
  lf::thread_context<vector_ctx> = ctx.get();
  lf::thread_context<lf::polymorphic_context> = ctx.get();

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

  if (tls_bump_ptr != buf.get()) {
    LF_TERMINATE("Stack leak detected");
  }
}

} // namespace

// Return by ref-arg, test direct root, no co-await, direct resumes, uses new/delete for alloc
BENCHMARK(fib<no_await<stack_on_heap>>)->Name("test/libfork/fib/heap/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<stack_on_heap>>)->Name("base/libfork/fib/heap/no_await")->Arg(fib_base);

// Same as above but uses bump allocator
BENCHMARK(fib<no_await<tls_bump>>)->Name("test/libfork/fib/bump_alloc/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<tls_bump>>)->Name("base/libfork/fib/bump_alloc/no_await")->Arg(fib_base);

// TODO: no_await with segmented stack allocator?

// Return by ref-arg, libfork call/call with co-await, uses new/delete for alloc
BENCHMARK(fib<await<stack_on_heap>>)->Name("test/libfork/fib/heap/await")->Arg(fib_test);
BENCHMARK(fib<await<stack_on_heap>>)->Name("base/libfork/fib/heap/await")->Arg(fib_base);

// Same as above but uses bump allocator
BENCHMARK(fib<await<tls_bump>>)->Name("test/libfork/fib/bump_alloc/await")->Arg(fib_test);
BENCHMARK(fib<await<tls_bump>>)->Name("base/libfork/fib/bump_alloc/await")->Arg(fib_base);

// Same as above but return by value in lf::task
BENCHMARK(fib<ret>)->Name("test/libfork/fib/bump_alloc/return")->Arg(fib_test);
BENCHMARK(fib<ret>)->Name("base/libfork/fib/bump_alloc/return")->Arg(fib_base);

// Return by value
// libfork call/fork (no join)
// Non-polymorphic vector-backed context
BENCHMARK(fib<fork_call<vector_ctx>>)->Name("test/libfork/fib/vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<vector_ctx>>)->Name("base/libfork/fib/vector_ctx")->Arg(fib_base);

BENCHMARK(fib<fork_call<lf::polymorphic_context>>)->Name("test/libfork/fib/poly_vector_ctx")->Arg(fib_test);
BENCHMARK(fib<fork_call<lf::polymorphic_context>>)->Name("base/libfork/fib/poly_vector_ctx")->Arg(fib_base);
