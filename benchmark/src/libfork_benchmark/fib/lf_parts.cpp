#include <coroutine>
#include <cstddef>
#include <new>

#include <benchmark/benchmark.h>

#include "libfork_benchmark/fib/fib.hpp"

import libfork.core;

namespace {

struct stack_on_heap {
  static constexpr auto operator new(std::size_t sz) -> void * { return ::operator new(sz); }
  static constexpr auto operator delete(void *p) noexcept -> void { ::operator delete(p); }
};

struct stack_on_data {
  static constexpr std::size_t capacity = 1024 * 1024 * 4;
  thread_local static std::byte buffer[capacity];
  thread_local static std::size_t offset;

  static auto operator new(std::size_t sz) -> void * {
    sz = (sz + 15uz) & ~15uz;
    if (offset + sz > capacity) {
      throw std::bad_alloc();
    }
    void *ptr = &buffer[offset];
    offset += sz;
    return ptr;
  }

  static auto operator delete([[maybe_unused]] void *p) noexcept -> void {}

  static auto operator delete([[maybe_unused]] void *p, std::size_t sz) noexcept -> void {
    sz = (sz + 15uz) & ~15uz;
    offset -= sz;
  }
};

thread_local std::byte stack_on_data::buffer[stack_on_data::capacity];
thread_local std::size_t stack_on_data::offset = 0;

template <lf::alloc_mixin StackPolicy>
constexpr auto no_sym = [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, StackPolicy> {
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
constexpr auto sym = [](this auto fib, std::int64_t *ret, std::int64_t n) -> lf::task<void, StackPolicy> {
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

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;

    Fn(&result, n).release()->handle().resume();

    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCHMARK(fib<no_sym<stack_on_heap>>)->Name("test/libfork/fib/heap/no_sym_transfer")->Arg(fib_test);
BENCHMARK(fib<no_sym<stack_on_heap>>)->Name("base/libfork/fib/heap/no_sym_transfer")->Arg(fib_base);

BENCHMARK(fib<sym<stack_on_heap>>)->Name("test/libfork/fib/heap/sym_transfer")->Arg(fib_test);
BENCHMARK(fib<sym<stack_on_heap>>)->Name("base/libfork/fib/heap/sym_transfer")->Arg(fib_base);

BENCHMARK(fib<no_sym<stack_on_data>>)->Name("test/libfork/fib/data/no_sym_transfer")->Arg(fib_test);
BENCHMARK(fib<no_sym<stack_on_data>>)->Name("base/libfork/fib/data/no_sym_transfer")->Arg(fib_base);

BENCHMARK(fib<sym<stack_on_data>>)->Name("test/libfork/fib/data/sym_transfer")->Arg(fib_test);
BENCHMARK(fib<sym<stack_on_data>>)->Name("base/libfork/fib/data/sym_transfer")->Arg(fib_base);
