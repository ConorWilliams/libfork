#include <benchmark/benchmark.h>

#include "common.hpp"
#include "fib.hpp"
#include "macros.hpp"

#include <bit>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <memory>

import libfork;

// === Coroutine

namespace {

// ==== Allocators ==== //

[[nodiscard]]
inline auto fib_align_size(std::size_t n) -> std::size_t {
  return (n + __STDCPP_DEFAULT_NEW_ALIGNMENT__ - 1) & ~(__STDCPP_DEFAULT_NEW_ALIGNMENT__ - 1);
}

constinit inline thread_local std::byte *tls_bump_ptr = nullptr;

struct task {
  struct promise_type {

    static auto operator new(std::size_t sz) -> void * {
      auto *prev = tls_bump_ptr;
      tls_bump_ptr += fib_align_size(sz);
      return prev;
    }

    static auto operator delete(void *p, [[maybe_unused]] std::size_t sz) noexcept -> void {
      tls_bump_ptr = std::bit_cast<std::byte *>(p);
    }

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
  co_await fib(n - 2).set(a);
  co_await fib(n - 1).set(b);
  co_return a + b;
}

template <typename = void>
void fib_coro_no_queue(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  // 8MB stack
  std::unique_ptr buffer = std::make_unique<std::byte[]>(1024 * 1024 * 8);
  tls_bump_ptr = buffer.get();

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;
    fib(n).set(result).coro.resume();

    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }

    benchmark::DoNotOptimize(result);
  }

  if (tls_bump_ptr != buffer.get()) {
    std::terminate(); // Stack leak
  }
}

// === Recursive with Deque overhead

constinit inline thread_local lf::deque<std::int64_t> *tls_deque = nullptr;

auto deque() -> lf::deque<std::int64_t> & { return *tls_deque; }

auto fib_recursive_deque_impl(std::int64_t n) -> std::int64_t {
  if (n <= 1) {
    return n;
  }

  // Emulate work item creation/scheduling overhead
  deque().push(n);
  std::int64_t a = fib_recursive_deque_impl(n - 2);
  deque().pop();

  std::int64_t b = fib_recursive_deque_impl(n - 1);

  return a + b;
}

template <typename = void>
void fib_recursive_deque(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  lf::deque<std::int64_t> deque{64};
  tls_deque = &deque;

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = fib_recursive_deque_impl(n);

    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }

    benchmark::DoNotOptimize(result);
  }

  tls_deque = nullptr;
}

} // namespace

// Minimal coroutine, bump allocated (thread-local) stack
BENCH_ALL(fib_coro_no_queue, baremetal, coro, fib)

BENCH_ALL(fib_recursive_deque, baremetal, deque, fib)
