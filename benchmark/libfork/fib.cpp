
#include "../bench.hpp"

#include "libfork/core.hpp"
#include "libfork/schedule/busy.hpp"

using namespace lf;

inline constexpr async_fn fib = [](auto fib, int n) -> task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(a, fib)(n - 1);
  co_await lf::call(b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

void run(std::string name, int x) {
  benchmark(name, [&](std::size_t n, auto &&bench) {
    // Set up
    auto pool = lf::busy_pool{n};

    int answer = 0;

    bench([&] {
      answer = lf::sync_wait(pool, fib, x);
    });

    return answer;
  });
}

auto main() -> int {
  run("fork-fib-05", 5);
  run("fork-fib-10", 10);
  run("fork-fib-15", 15);
  run("fork-fib-20", 20);
  run("fork-fib-25", 25);
  run("fork-fib-30", 30);
  return 0;
}