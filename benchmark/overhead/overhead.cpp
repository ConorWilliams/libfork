#include <nanobench.h>

#include "libfork/libfork.hpp"
#include "libfork/schedule/busy.hpp"
#include "libfork/schedule/inline.hpp"

namespace lf::detail {

struct make_fn_impl {
  template <stateless Fn>
  [[nodiscard]] consteval auto operator+([[maybe_unused]] Fn invocable_which_returns_a_task) const -> async_fn<Fn> { return {}; }
};

inline constexpr make_fn_impl make_fn;

}; // namespace lf::detail

// NOLINTBEGIN

/**
 * @brief Macro to automate the creation of an async function with the first argument ``auto self``.
 */
#define ASYNC(...) ::lf::detail::make_fn + [](auto self __VA_OPT__(, ) __VA_ARGS__) LF_STATIC_CALL

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

inline constexpr auto c_fib_invoke = ASYNC(int n) -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  co_return co_await self(n - 1) + co_await self(n - 2);
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

  bench.title("Fibonacci");
  bench.warmup(100);
  bench.relative(true);
  bench.performanceCounters(true);
  // bench.minEpochIterations(10);

  volatile int in = 30;

  for (std::size_t i = 1; i <= 4; ++i) {
    lf::busy_pool i_sch{i};

    bench.run("async busy pool n=" + std::to_string(i), [&] {
      ankerl::nanobench::doNotOptimizeAway(lf::sync_wait(i_sch, c_fib, in));
    });
  }

  {
    lf::inline_scheduler i_sch;

    bench.run("async inline", [&] {
      ankerl::nanobench::doNotOptimizeAway(lf::sync_wait(i_sch, c_fib, in));
    });
  }

  bench.run("function no register", [&] {
    ankerl::nanobench::doNotOptimizeAway(fib_no_reg({in}));
  });

  bench.run("function", [&] {
    ankerl::nanobench::doNotOptimizeAway(fib(in));
  });

  return 0;
}
