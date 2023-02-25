
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

void run(std::string name, int x) {
  benchmark(name, [&](std::size_t n, auto&& bench) {
    // Set up
    auto pool = busy_pool{n};

    int answer = 0;

    bench([&] {
      answer = pool.schedule(fib<busy_pool::context>(x));
    });

    return answer;
  });
}

auto main() -> int {
  run("libfork, fib 5", 5);
  run("libfork, fib 10", 10);
  run("libfork, fib 15", 15);
  run("libfork, fib 20", 20);
  run("libfork, fib 25", 25);
  run("libfork, fib 30", 30);
  return 0;
}