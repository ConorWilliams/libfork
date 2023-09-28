

// #undef NDEBUG
// #define LF_DEFAULT_LOGGING

#include <iostream>

#include <benchmark/benchmark.h>

#include <libfork.hpp>
#include <threads.h>

inline constexpr int work = 32;
volatile int output;

// ----------------------- Baseline ----------------------- //

namespace serial {

LF_NOINLINE constexpr auto fib_impl(int &ret, int n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  int a, b;

  fib_impl(a, n - 1);
  fib_impl(b, n - 2);

  ret = a + b;
};

void fib(benchmark::State &state) {

  for (auto _ : state) {
    int tmp;
    fib_impl(tmp, work);
    output = tmp;
  }
}

} // namespace serial

BENCHMARK(serial::fib);

// ----------------------- Zero coro ----------------------- //

namespace heap {

struct promise;

struct coroutine : lf::stdx::coroutine_handle<promise> {
  using promise_type = heap::promise;
};

struct promise {
  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> lf::stdx::suspend_always { return {}; }
  static auto final_suspend() noexcept -> lf::stdx::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false); }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  fib_impl(a, n - 1).resume();
  fib_impl(b, n - 2).resume();

  ret = a + b;
};

void fib(benchmark::State &state) {

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }
}

} // namespace heap

BENCHMARK(heap::fib);

// ----------------------- custom Stackfull coro ----------------------- //

namespace custom_stack {

thread_local std::byte *asp;

struct promise;

struct coroutine : lf::stdx::coroutine_handle<promise> {
  using promise_type = custom_stack::promise;
};

struct op {

  static constexpr auto align = 2 * sizeof(void *);

  [[nodiscard]] LF_TLS_CLANG_INLINE static auto operator new(std::size_t size) -> void * {
    return asp += (size + align - 1) & ~(align - 1);
  }

  LF_TLS_CLANG_INLINE static void operator delete(void *ptr) { asp = static_cast<std::byte *>(ptr); }
};

struct promise : op {

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> lf::stdx::suspend_always { return {}; }
  static auto final_suspend() noexcept -> lf::stdx::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false); }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  fib_impl(a, n - 1).resume();
  fib_impl(b, n - 2).resume();

  ret = a + b;
};

void fib(benchmark::State &state) {

  std::vector<std::byte> stack(1024 * 1024);

  asp = stack.data();

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }
}

} // namespace custom_stack

BENCHMARK(custom_stack::fib);

// ----------------------- Stackfull coro ----------------------- //

namespace stack_lf {

struct promise;

struct coroutine : lf::stdx::coroutine_handle<promise> {
  using promise_type = stack_lf::promise;
};

struct promise : lf::impl::promise_alloc_stack {

  promise() : promise_alloc_stack(coroutine::from_promise(*this)) {}

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> lf::stdx::suspend_always { return {}; }
  static auto final_suspend() noexcept -> lf::stdx::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false); }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  fib_impl(a, n - 1).resume();
  fib_impl(b, n - 2).resume();

  ret = a + b;
};

void fib(benchmark::State &state) {

  auto *stack = new lf::async_stack;

  lf::impl::tls::set_asp(lf::impl::stack_as_bytes(stack));

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }

  if (lf::impl::tls::asp != lf::impl::stack_as_bytes(stack)) {
    LF_THROW("stack leak");
  }

  delete stack;
}

} // namespace stack_lf

BENCHMARK(stack_lf::fib);

// ----------------------- Stackfull coro ----------------------- //

namespace transfer {

struct promise;

struct coroutine {
  using promise_type = transfer::promise;
  lf::impl::frame_block *prom;
};

struct promise : lf::impl::promise_alloc_stack {

  promise() : promise_alloc_stack(lf::stdx::coroutine_handle<promise>::from_promise(*this)) {}

  auto get_return_object() -> coroutine { return coroutine{this}; }

  static auto initial_suspend() noexcept { return lf::stdx::suspend_always{}; }

  static auto final_suspend() noexcept {
    struct awaitable : lf::stdx::suspend_always {
      auto await_suspend(lf::stdx::coroutine_handle<promise> self) noexcept -> lf::stdx::coroutine_handle<> {
        if (self.promise().is_root()) {
          self.destroy();
          return lf::stdx::noop_coroutine();
        }
        frame_block *parent = self.promise().parent();
        self.destroy();
        return parent->coro();
      }
    };

    return awaitable{};
  }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false); }

  auto await_transform(coroutine child) {

    child.prom->set_parent(this);

    struct awaitable : lf::stdx::suspend_always {

      auto await_suspend(lf::stdx::coroutine_handle<promise>) { return child->coro(); }

      frame_block *child;
    };

    return awaitable{{}, child.prom};
  }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  co_await fib_impl(a, n - 1);
  co_await fib_impl(b, n - 2);

  ret = a + b;
};

void fib(benchmark::State &state) {

  auto *stack = new lf::async_stack;

  lf::impl::tls::set_asp(lf::impl::stack_as_bytes(stack));

  for (auto _ : state) {
    int tmp;
    fib_impl(tmp, work).prom->coro().resume();
    output = tmp;
  }

  delete stack;
}

} // namespace transfer

BENCHMARK(transfer::fib);

// ----------------------- libfork ----------------------- //

namespace libfork {

inline constexpr lf::async fib_impl = [](auto fib, int n) LF_STATIC_CALL -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, fib)(n - 1);
  co_await lf::call(b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

template <typename Sch>
void fib(benchmark::State &state) {

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(1);
    } else {
      return Sch{};
    }
  }();

  for (auto _ : state) {
    output = lf::sync_wait(sch, fib_impl, work);
  }
}

} // namespace libfork

BENCHMARK(libfork::fib<lf::unit_pool>)->UseRealTime();
BENCHMARK(libfork::fib<lf::debug_pool>)->UseRealTime();
BENCHMARK(libfork::fib<lf::busy_pool>)->UseRealTime();

void deque(benchmark::State &state) {

  lf::deque<void *> data;

  volatile int range = 2000;

  for (auto _ : state) {

    int lim = range;

    for (int i = 0; i < lim; ++i) {
      data.push(&data);
    }

    while (data.pop()) {
    }
  }
}

BENCHMARK(deque);
