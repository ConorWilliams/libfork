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

thread_local static std::byte *sp = nullptr;

[[nodiscard]]
auto align(std::size_t n) -> std::size_t {
  return (n + lf::k_new_align - 1) & ~(lf::k_new_align - 1);
}

struct tls_stack {

  static auto operator new(std::size_t sz) -> void * {
    auto *prev = sp;
    sp += align(sz);
    return prev;
  }

  static auto operator delete([[maybe_unused]] void *p, [[maybe_unused]] std::size_t sz) noexcept -> void {
    sp = std::bit_cast<std::byte *>(p);
  }
};

template <lf::alloc_mixin StackPolicy>
constexpr auto no_await =
    [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, StackPolicy> {
  if (n < 2) {
    *ret = n;
    co_return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  fib(&lhs, n - 1).release()->handle().resume();
  fib(&rhs, n - 2).release()->handle().resume();

  *ret = lhs + rhs;
};

template <lf::alloc_mixin StackPolicy>
constexpr auto await = [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, StackPolicy> {
  if (n < 2) {
    *ret = n;
    co_return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  co_await fib(&lhs, n - 1);
  co_await fib(&rhs, n - 2);

  *ret = lhs + rhs;
};

template <auto Fn>
void fib(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  std::unique_ptr buffer = std::make_unique<std::byte[]>(1024 * 1024);

  sp = buffer.get();

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;

    Fn(&result, n).release()->handle().resume();

    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }

  if (sp != buffer.get()) {
    LF_TERMINATE("Stack leak detected");
  }
}

} // namespace

BENCHMARK(fib<no_await<stack_on_heap>>)->Name("test/libfork/fib/heap/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<stack_on_heap>>)->Name("base/libfork/fib/heap/no_await")->Arg(fib_base);

BENCHMARK(fib<await<stack_on_heap>>)->Name("test/libfork/fib/heap/await")->Arg(fib_test);
BENCHMARK(fib<await<stack_on_heap>>)->Name("base/libfork/fib/heap/await")->Arg(fib_base);

BENCHMARK(fib<no_await<tls_stack>>)->Name("test/libfork/fib/data/no_await")->Arg(fib_test);
BENCHMARK(fib<no_await<tls_stack>>)->Name("base/libfork/fib/data/no_await")->Arg(fib_base);

BENCHMARK(fib<await<tls_stack>>)->Name("test/libfork/fib/data/await")->Arg(fib_test);
BENCHMARK(fib<await<tls_stack>>)->Name("base/libfork/fib/data/await")->Arg(fib_base);
