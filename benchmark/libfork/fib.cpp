
#include "../bench.hpp"

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

using namespace lf;

template <context Context>
auto fib(int n) -> basic_task<int, Context> {
  if (n < 2) {
    co_return n;
  }

  auto a = co_await fib<Context>(n - 1).fork();
  auto b = co_await fib<Context>(n - 2);

  co_await join();

  co_return *a + b;
}

auto main() -> int {
  benchmark("libfork, fib", [](std::size_t n, auto&& bench) {
    // Set up
    auto pool = busy_pool{n};

    int ans = 0;

    bench([&] {
      ans = pool.schedule(fib<busy_pool::context>(25));
    });

    return ans;
  });

  return 0;
}