#include <bit>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <print>

#include <benchmark/benchmark.h>

void sink(int &x) { benchmark::DoNotOptimize(x); }

// === Coroutine

template <typename T = void>
using handle = std::coroutine_handle<T>;

namespace {

constexpr std::size_t k_new_align = 2 * sizeof(void *);
constexpr std::size_t k_stack_size = 1024 * 1024 * 8;

static constexpr auto align(std::size_t n) -> std::size_t {
  return (n + k_new_align - 1) & ~(k_new_align - 1);
}

static thread_local unsigned char *stk = nullptr;

struct global_fixed {

  static auto operator new(std::size_t sz) -> void * {
    auto *tmp_stk = stk;
    stk += align(sz);
    return tmp_stk;
  }

  static auto operator delete(void *p) -> void { stk = static_cast<unsigned char *>(p); }
};

template <typename Mixin>
struct task_of {
  struct promise_type : Mixin {

    auto get_return_object() -> task_of { return {handle<promise_type>::from_promise(*this)}; }

    auto initial_suspend() -> std::suspend_always { return {}; }

    auto final_suspend() noexcept {
      struct final_awaitable : std::suspend_always {
        auto await_suspend(handle<promise_type> h) noexcept -> handle<> {

          handle continue_ = h.promise().continue_;

          h.destroy();

          if (continue_) {
            return continue_;
          }

          return std::noop_coroutine();
        }
      };

      return final_awaitable{};
    }

    void return_value(int value) { *value_ = value; }
    void unhandled_exception() { std::terminate(); }

    int *value_;
    std::coroutine_handle<> continue_ = nullptr;
  };

  std::coroutine_handle<promise_type> coro_;

  auto set(int &out) -> task_of & {
    coro_.promise().value_ = &out;
    return *this;
  }

  bool await_ready() noexcept { return false; }

  auto await_suspend(handle<> h) -> handle<promise_type> {
    coro_.promise().continue_ = h;
    return coro_;
  }

  void await_resume() noexcept {}
};

using task = task_of<global_fixed>;

auto fib(int n) -> task {
  if (n <= 1) {
    co_return n;
  }
  int a, b;
  co_await fib(n - 1).set(a);
  co_await fib(n - 2).set(b);
  co_return a + b;
}

void fib_coro_no_queue(benchmark::State &state) {

  stk = new unsigned char[k_stack_size];

  for (auto _ : state) {
    int x;
    fib(n_fib).set(x).coro_.resume();
    sink(x);
  }

  delete[] stk;
}

inline thread_local lf::deque<int *> *ctx;

template <typename Mixin>
auto fib_q(int n) -> task_of<Mixin> {
  if (n <= 1) {
    co_return n;
  }

  int a, b;

  ctx->push(&a);
  co_await fib_q<Mixin>(n - 1).set(a);
  ctx->pop();

  ctx->push(&b);
  co_await fib_q<Mixin>(n - 2).set(b);
  ctx->pop();

  co_return a + b;
}

void fib_coro_with_queue(benchmark::State &state) {

  stk = new unsigned char[k_stack_size];
  lf::deque<int *> q;
  ctx = &q;

  for (auto _ : state) {
    int x;
    fib_q<global_fixed>(n_fib).set(x).coro_.resume();
    sink(x);
  }

  delete[] stk;
}

auto fib_fn_with_queue(int &x, int n) {
  if (n <= 1) {
    x = n;
    return;
  }
  int a, b;

  ctx->push(&a);
  fib_fn_with_queue(a, n - 1);
  ctx->pop();

  ctx->push(&b);
  fib_fn_with_queue(b, n - 2);
  ctx->pop();

  x = a + b;
}

void fib_with_queue(benchmark::State &state) {

  lf::deque<int *> q;
  ctx = &q;

  for (auto _ : state) {
    int x;
    fib_fn_with_queue(x, n_fib);
    sink(x);
  }
}

} // namespace

BENCHMARK(fib_with_queue);
BENCHMARK(fib_coro_no_queue);
BENCHMARK(fib_coro_with_queue);

namespace {

inline thread_local memory::stacklet *_stl = nullptr;
inline thread_local std::byte *_sp = nullptr;

struct mixin_dynstack {
  static auto operator new(std::size_t sz) -> void * {
    // std::println("allocating {}", sz);
    auto [stl, ex, sp] = memory::push(_stl, _sp, sz);
    _stl = stl;
    _sp = sp;
    return ex;
  }

  static auto operator delete(void *p) -> void {
    // std::println("deallocating");
    auto [stl, sp] = memory::pop(_stl, std::bit_cast<std::byte *>(p), 0);
    _stl = stl;
    _sp = sp;
  }
};

void a_simple_queue(benchmark::State &state) {
  _stl = memory::new_stacklet(nullptr);
  _sp = _stl->data.data();
  lf::deque<int *> q;
  ctx = &q;

  for (auto _ : state) {
    int x;
    fib_q<mixin_dynstack>(n_fib).set(x).coro_.resume();
    sink(x);
  }

  delete[] _stl;
}

} // namespace

BENCHMARK(a_simple_queue);
