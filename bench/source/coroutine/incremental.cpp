

// #undef NDEBUG
// #define LF_DEFAULT_LOGGING

#include <iostream>

#include <benchmark/benchmark.h>

#include <libfork.hpp>
#include <libfork/core/ext/tls.hpp>
#include <libfork/core/impl/promise.hpp>
#include <libfork/core/macro.hpp>

inline constexpr volatile int work = 24;
volatile int output;

// ----------------------- Baseline ----------------------- //

// Fibonacci as fast as possible single threaded.

namespace return_serial {

LF_NOINLINE constexpr auto fib_impl(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib_impl(n - 1) + fib_impl(n - 2);
}

void fib(benchmark::State &state) {
  for (auto _ : state) {
    output = fib_impl(work);
  }
}

} // namespace return_serial

BENCHMARK(return_serial::fib);

// Fibonacci single threaded with out of line return to emulate fastest possible
// when not returning via a register.

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
}

void fib(benchmark::State &state) {

  for (auto _ : state) {
    int tmp;
    fib_impl(tmp, work);
    output = tmp;
  }
}

} // namespace serial

BENCHMARK(serial::fib);

// ----------------------- Baseline ----------------------- //

// Repeat serial but with fictitious push/pop to lock free queue
// This is the lower-bound of performance for fork-join if we manage to get the
// coroutine overhead to zero.

lf::deque<int *> qu;

namespace serial_queue {

LF_NOINLINE constexpr auto fib_impl(int &ret, int n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  int a;
  int b;

  qu.push(&a);

  fib_impl(a, n - 1);
  fib_impl(b, n - 2);

  qu.pop();

  ret = a + b;
}

void fib(benchmark::State &state) {

  for (auto _ : state) {
    int tmp;
    fib_impl(tmp, work);
    output = tmp;
  }
}

} // namespace serial_queue

BENCHMARK(serial_queue::fib);

// ----------------------- Zero coro ----------------------- //

// This is bare minimal coroutine that allocates on the heap.
// When compared with serial::fib this included heap allocation
// + coroutine cost.

namespace heap_alloc_coro {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = heap_alloc_coro::promise;
};

struct promise {
  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
  static auto final_suspend() noexcept -> std::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
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
}

void fib(benchmark::State &state) {

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }
}

} // namespace heap_alloc_coro

BENCHMARK(heap_alloc_coro::fib);

// ----------------------- custom Stackfull coro ----------------------- //

// This is bare minimal coroutine that allocates single fixed stack.
// When compared with serial::fib this minimal coroutine cost.

namespace fixed_stack_coro {

thread_local std::byte *asp;

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = fixed_stack_coro::promise;
};

struct op {

  static constexpr auto align = 2 * sizeof(void *);

  [[nodiscard]] static auto operator new(std::size_t size) -> void * {
    auto *prev = asp;
    asp += (size + align - 1) & ~(align - 1);
    return prev;
  }

  static void operator delete(void *ptr) { asp = static_cast<std::byte *>(ptr); }
};

struct promise : /*lf::impl::frame_block, */ op {

  // double f[40]{};

  /*promise() : lf::impl::frame_block(coroutine::from_promise(*this), nullptr) {}*/

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_never { return {}; }
  static auto final_suspend() noexcept -> std::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  fib_impl(a, n - 1);
  fib_impl(b, n - 2);

  ret = a + b;
}

void fib(benchmark::State &state) {

  std::vector<std::byte> stack(1024 * 1024);

  asp = stack.data();

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    output = tmp;
  }
}

} // namespace fixed_stack_coro

BENCHMARK(fixed_stack_coro::fib);

// ----------------------- custom Stackfull coro ----------------------- //

// This is bare minimal coroutine that allocates single fixed stack.
// When compared with serial::fib this minimal coroutine cost.

namespace fixed_stack_coro_lazy {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = fixed_stack_coro_lazy::promise;
};

struct promise : /*lf::impl::frame_block, */ fixed_stack_coro::op {

  // double f[40]{};

  /*promise() : lf::impl::frame_block(coroutine::from_promise(*this), nullptr) {}*/

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
  static auto final_suspend() noexcept -> std::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
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
}

void fib(benchmark::State &state) {

  std::vector<std::byte> stack(1024 * 1024);

  fixed_stack_coro::asp = stack.data();

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }
}

} // namespace fixed_stack_coro_lazy

BENCHMARK(fixed_stack_coro_lazy::fib);

// ----------------------- custom Stackfull coro ----------------------- //

// This is bare minimal coroutine that allocates single fixed stack and
// includes a lf::impl::frame to emulate the cost of a fork-join with
// worse cache locality than a single threaded version.

namespace fixed_stack_coro_with_frame {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = fixed_stack_coro_with_frame::promise;
};

struct promise : lf::impl::frame, fixed_stack_coro::op {

  // double f[40]{};

  promise() : lf::impl::frame(coroutine::from_promise(*this), nullptr) {}

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
  static auto final_suspend() noexcept -> std::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
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
}

void fib(benchmark::State &state) {

  std::vector<std::byte> stack(1024 * 1024);

  fixed_stack_coro::asp = stack.data();

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }
}

} // namespace fixed_stack_coro_with_frame

BENCHMARK(fixed_stack_coro_with_frame::fib);

// ----------------------- custom Stackfull coro ----------------------- //

// A variation fixed_stack_coro_with_frame but makes a TLS call to get stack.

namespace fixed_stack_coro_with_frame_tls {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = fixed_stack_coro_with_frame_tls::promise;
};

struct promise : lf::impl::frame, fixed_stack_coro::op {

  // double f[40]{};

  promise() : lf::impl::frame(coroutine::from_promise(*this), lf::impl::tls::stack()->top()) {}

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
  static auto final_suspend() noexcept -> std::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
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
}

void fib(benchmark::State &state) {

  std::vector<std::byte> stack(1024 * 1024);

  auto *ctx = lf::worker_init(lf::nullary_function_t{[]() {}});

  fixed_stack_coro::asp = stack.data();

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }

  lf::finalize(ctx);
}

} // namespace fixed_stack_coro_with_frame_tls

BENCHMARK(fixed_stack_coro_with_frame_tls::fib);

// ----------------------- custom Stackfull coro ----------------------- //

// This is bare minimal coroutine that allocates a single fixed stack and
// includes a lf::impl::frame in the promise, this uses suspend_always
// at final suspend and calls destroy manually at the coroutine call site.

namespace fixed_stack_coro_with_frame_no_term {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = struct promise;
};

struct promise : lf::impl::frame, fixed_stack_coro::op {

  promise() : lf::impl::frame(coroutine::from_promise(*this), nullptr) {}

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
  static auto final_suspend() noexcept -> std::suspend_always { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a, b;

  {
    auto h = fib_impl(a, n - 1);
    h.resume();
    h.destroy();
  }

  {
    auto h = fib_impl(b, n - 2);
    h.resume();
    h.destroy();
  }

  ret = a + b;
}

void fib(benchmark::State &state) {

  std::vector<std::byte> stack(1024 * 1024);

  fixed_stack_coro::asp = stack.data();

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    h.destroy();
    output = tmp;
  }
}

} // namespace fixed_stack_coro_with_frame_no_term

BENCHMARK(fixed_stack_coro_with_frame_no_term::fib);

// ----------------------- Stackfull coro ----------------------- //

// This is bare minimal coroutine that uses a dynamic stack and includes a
// lf::impl::frame in the promise.

namespace segmented_stack {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine : std::coroutine_handle<promise> {
  using promise_type = segmented_stack::promise;
};

struct promise : lf::impl::promise_base {

  promise() : lf::impl::promise_base(coroutine::from_promise(*this), lf::impl::tls::stack()->top()) {}

  auto get_return_object() -> coroutine { return {coroutine::from_promise(*this)}; }
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
  static auto final_suspend() noexcept -> std::suspend_never { return {}; }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }
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
}

void fib(benchmark::State &state) {

  auto *ctx = lf::worker_init(lf::nullary_function_t{[]() {}});

  for (auto _ : state) {
    int tmp;
    auto h = fib_impl(tmp, work);
    h.resume();
    output = tmp;
  }

  lf::finalize(ctx);
}

} // namespace segmented_stack

BENCHMARK(segmented_stack::fib);

// ----------------------- Stackfull coro ----------------------- //

// Dynamic stack + co_await to symmetric transfer into children.

namespace segmented_stack_co_await {

struct promise;

struct LF_CORO_ATTRIBUTES coroutine {
  using promise_type = segmented_stack_co_await::promise;
  lf::impl::frame *prom;
};

struct promise : lf::impl::promise_base {

  promise()
      : lf::impl::promise_base(std::coroutine_handle<promise>::from_promise(*this),
                               lf::impl::tls::stack()->top()) {}

  auto get_return_object() -> coroutine { return coroutine{this}; }

  static auto initial_suspend() noexcept { return std::suspend_always{}; }

  static auto final_suspend() noexcept {
    struct awaitable : std::suspend_always {
      auto await_suspend(std::coroutine_handle<promise> self) noexcept -> std::coroutine_handle<> {

        frame *parent = self.promise().parent();
        self.destroy();

        if (parent != nullptr) {
          return parent->self();
        }
        return std::noop_coroutine();
      }
    };

    return awaitable{};
  }
  static void return_void() {}
  static void unhandled_exception() { LF_ASSERT(false && "Unreachable"); }

  auto await_transform(coroutine child) {

    child.prom->set_parent(this);

    struct awaitable : std::suspend_always {

      auto await_suspend(std::coroutine_handle<promise>) { return child->self(); }

      frame *child;
    };

    return awaitable{{}, child.prom};
  }
};

auto fib_impl(int &ret, int n) -> coroutine {
  if (n < 2) {
    ret = n;
    co_return;
  }

  int a;
  int b;

  co_await fib_impl(a, n - 1);
  co_await fib_impl(b, n - 2);

  ret = a + b;
}

void fib(benchmark::State &state) {

  auto *ctx = lf::worker_init(lf::nullary_function_t{[]() {}});

  for (auto _ : state) {
    int tmp = 0;

    lf::impl::frame *root = fib_impl(tmp, work).prom;
    root->set_parent(nullptr);
    root->self().resume();

    output = tmp;
  }

  lf::finalize(ctx);
}

} // namespace segmented_stack_co_await

BENCHMARK(segmented_stack_co_await::fib);

// ----------------------- segmented_stack_libfork ----------------------- //

// Libfork all co_awaits are on lf::call

namespace segmented_stack_libfork {

inline constexpr auto fib_impl = [](auto fib, int n) LF_STATIC_CALL -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  int a;
  int b;

  co_await lf::call(&a, fib)(n - 1);
  co_await lf::call(&b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

void fib(benchmark::State &state) {
  for (auto _ : state) {
    output = lf::sync_wait(lf::unit_pool{}, fib_impl, work);
  }
}

} // namespace segmented_stack_libfork

BENCHMARK(segmented_stack_libfork::fib);

// ----------------------- libfork void ----------------------- //

namespace libfork_forking_return_void {

inline constexpr auto fib_impl = [](auto fib, int &res, int n) LF_STATIC_CALL -> lf::task<void> {
  if (n < 2) {
    res = n;
    co_return;
  }

  int a, b;

  co_await lf::fork(fib)(a, n - 1);
  co_await lf::call(fib)(b, n - 2);

  co_await lf::join;

  res = a + b;
};

void fib(benchmark::State &state) {
  for (auto _ : state) {
    int tmp;
    lf::sync_wait(lf::unit_pool{}, fib_impl, tmp, work);
    output = tmp;
  }
}

} // namespace libfork_forking_return_void

BENCHMARK(libfork_forking_return_void::fib);

// ----------------------- libfork ----------------------- //

namespace libfork_forking_return_int {

inline constexpr auto fib_impl = [](auto fib, int n) LF_STATIC_CALL -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  // clang-format off

  LF_TRY {
    co_await lf::fork(&a, fib)(n - 1);
    co_await lf::call(&b, fib)(n - 2);
  } LF_CATCH_ALL { 
    fib.stash_exception(); 
  }

  // clang-format on

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

} // namespace libfork_forking_return_int

BENCHMARK(libfork_forking_return_int::fib<lf::unit_pool>)->UseRealTime();
BENCHMARK(libfork_forking_return_int::fib<lf::lazy_pool>)->UseRealTime();
BENCHMARK(libfork_forking_return_int::fib<lf::busy_pool>)->UseRealTime();

// ----------------------- libfork exception safe ----------------------- //

namespace libfork_forking_return_int_exceptions {

inline constexpr auto fib_impl = [](auto fib, int n) LF_STATIC_CALL -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  LF_TRY {
    co_await lf::fork(&a, fib)(n - 1);
    co_await lf::call(&b, fib)(n - 2);
    goto fallthrough;
  }
  LF_CATCH_ALL {}

fallthrough:
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

} // namespace libfork_forking_return_int_exceptions

BENCHMARK(libfork_forking_return_int_exceptions::fib<lf::unit_pool>)->UseRealTime();
BENCHMARK(libfork_forking_return_int_exceptions::fib<lf::lazy_pool>)->UseRealTime();
BENCHMARK(libfork_forking_return_int_exceptions::fib<lf::busy_pool>)->UseRealTime();

void deque(benchmark::State &state) {

  lf::deque<void *> data;

  volatile int range = 20000;

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

void vec(benchmark::State &state) {

  std::vector<void *> data;

  volatile int range = 20000;

  for (auto _ : state) {

    int lim = range;

    for (int i = 0; i < lim; ++i) {
      data.push_back(&data);
    }

    while (!data.empty()) {
      data.pop_back();
    }
  }
}

BENCHMARK(vec);
