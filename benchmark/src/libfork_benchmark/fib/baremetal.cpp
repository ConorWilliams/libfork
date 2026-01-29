#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>

#include <benchmark/benchmark.h>

#include "libfork_benchmark/fib/fib.hpp"

// === Coroutine

namespace {

struct task {
  struct promise_type : fib_bump_allocator {

    auto get_return_object() -> task { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }

    auto initial_suspend() -> std::suspend_always { return {}; }

    auto final_suspend() noexcept {
      struct final_awaitable : std::suspend_always {
        auto await_suspend(std::coroutine_handle<promise_type> h) noexcept -> std::coroutine_handle<> {

          std::coroutine_handle<> cont = h.promise().continuation;

          h.destroy();

          if (cont) {
            return cont;
          }

          return std::noop_coroutine();
        }
      };

      return final_awaitable{};
    }

    void return_value(std::int64_t val) { *value = val; }
    void unhandled_exception() { std::terminate(); }

    std::int64_t *value = nullptr;
    std::coroutine_handle<> continuation = nullptr;
  };

  std::coroutine_handle<promise_type> coro;

  auto set(std::int64_t &out) -> task & {
    coro.promise().value = &out;
    return *this;
  }

  auto await_ready() noexcept -> bool { return false; }

  auto await_suspend(std::coroutine_handle<> h) -> std::coroutine_handle<promise_type> {
    coro.promise().continuation = h;
    return coro;
  }

  void await_resume() noexcept {}
};

auto fib(std::int64_t n) -> task {
  if (n <= 1) {
    co_return n;
  }
  std::int64_t a = 0;
  std::int64_t b = 0;
  co_await fib(n - 1).set(a);
  co_await fib(n - 2).set(b);
  co_return a + b;
}

void fib_coro_no_queue(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  // 8MB stack
  std::unique_ptr buffer = std::make_unique<std::byte[]>(1024 * 1024 * 8);
  fib_bump_ptr = buffer.get();

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;
    fib(n).set(result).coro.resume();
    CHECK_RESULT(result, expect);
    benchmark::DoNotOptimize(result);
  }

  if (fib_bump_ptr != buffer.get()) {
    std::terminate(); // Stack leak
  }
}

} // namespace

BENCHMARK(fib_coro_no_queue)->Name("test/baremetal/fib")->Arg(fib_test);
BENCHMARK(fib_coro_no_queue)->Name("base/baremetal/fib")->Arg(fib_base);
