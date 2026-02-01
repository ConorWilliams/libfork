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

  fib(&lhs, n - 1).promise->handle().resume();
  fib(&rhs, n - 2).promise->handle().resume();

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

template <auto Fn>
void fib(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  std::unique_ptr buffer = std::make_unique<std::byte[]>(1024 * 1024);

  fib_bump_ptr = buffer.get();

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;

    if constexpr (requires { Fn(&result, n); }) {
      Fn(&result, n).promise->handle().resume();
    } else {
      auto task = Fn(n);
      task.promise->return_address = &result;
      task.promise->handle().resume();
    }

    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }

  if (fib_bump_ptr != buffer.get()) {
    LF_TERMINATE("Stack leak detected");
  }
}

template <lf::alloc_mixin Stack>
constexpr auto ret = [](this auto fib, std::int64_t n) -> lf::task<std::int64_t, Stack> {
  if (n < 2) {
    co_return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await lf::call(&lhs, fib, n - 1);
  co_await lf::call(&rhs, fib, n - 2);

  co_return lhs + rhs;
};

} // namespace

BENCHMARK(fib<no_await<stack_on_heap>>)->Name("test/libfork/fib/heap/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<stack_on_heap>>)->Name("base/libfork/fib/heap/no_await")->Arg(fib_base);

BENCHMARK(fib<await<stack_on_heap>>)->Name("test/libfork/fib/heap/await")->Arg(fib_test);
BENCHMARK(fib<await<stack_on_heap>>)->Name("base/libfork/fib/heap/await")->Arg(fib_base);

BENCHMARK(fib<no_await<fib_bump_allocator>>)->Name("test/libfork/fib/bump_alloc/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<fib_bump_allocator>>)->Name("base/libfork/fib/bump_alloc/no_await")->Arg(fib_base);

BENCHMARK(fib<await<fib_bump_allocator>>)->Name("test/libfork/fib/bump_alloc/await")->Arg(fib_test);
BENCHMARK(fib<await<fib_bump_allocator>>)->Name("base/libfork/fib/bump_alloc/await")->Arg(fib_base);

BENCHMARK(fib<ret<fib_bump_allocator>>)->Name("test/libfork/fib/bump_alloc/return")->Arg(fib_test);
BENCHMARK(fib<ret<fib_bump_allocator>>)->Name("base/libfork/fib/bump_alloc/return")->Arg(fib_base);
