#include <nanobench.h>

#include "libfork/libfork.hpp"
#include "libfork/schedule/inline.hpp"

__attribute__((noinline)) auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

  a = fib(n - 1);
  b = fib(n - 2);

  return a + b;
}

inline constexpr auto c_fib = ASYNC(int n) -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, self)(n - 1);
  co_await lf::call(b, self)(n - 2);

  co_await lf::join;

  co_return a + b;
};

struct no_register {
  int n;

  no_register() = default;
  no_register(int x) : n(x) {}
  no_register(no_register const &other) : n{other.n} {}
};

__attribute__((noinline)) auto fib_no_reg(no_register const &x) -> no_register {
  if (x.n < 2) {
    return x;
  }

  no_register a, b;

  a = fib_no_reg({x.n - 1});
  b = fib_no_reg({x.n - 2});

  no_register ret = {a.n + b.n};

  return std::move(ret);
}

auto main() -> int {
  //
  ankerl::nanobench::Bench bench;

  bench.title("Overhead");
  bench.warmup(100);
  bench.relative(true);
  bench.performanceCounters(true);

  volatile int in = 30;

  lf::inline_scheduler scheduler;

  bench.run("async inline", [&] {
    ankerl::nanobench::doNotOptimizeAway(lf::sync_wait(scheduler, c_fib, in));
  });

  bench.run("function no register", [&] {
    ankerl::nanobench::doNotOptimizeAway(fib_no_reg({in}));
  });

  bench.run("function", [&] {
    ankerl::nanobench::doNotOptimizeAway(fib(in));
  });

  return 0;
}
