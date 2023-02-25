#include "../bench.hpp"

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

  tbb::task_group g;

  g.run([&] {
    a = fib(n - 1);
  });

  b = fib(n - 2);

  g.wait();

  return a + b;
}

void run(std::string name, int x) {
  benchmark(name, [&](std::size_t n, auto&& bench) {
    int ans = 0;

    tbb::task_arena(n).execute([&] {
      bench([&] {
        ans = fib(x);
      });
    });

    return ans;
  });
}

auto main() -> int {
  run("tbb, fib 5", 5);
  run("tbb, fib 10", 10);
  run("tbb, fib 15", 15);
  run("tbb, fib 20", 20);
  run("tbb, fib 25", 25);
  run("tbb, fib 30", 30);
  return 0;
}
