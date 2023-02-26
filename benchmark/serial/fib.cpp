#include "../bench.hpp"

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

  a = fib(n - 1);

  b = fib(n - 2);

  return a + b;
}

void run(std::string name, int x) {
  benchmark(name, [&](std::size_t n, auto&& bench) {
    int answer = 0;

    bench([&] {
      answer = fib(x);
    });

    return answer;
  });
}

auto main() -> int {
  run("serial-fib-05", 5);
  run("serial-fib-10", 10);
  run("serial-fib-15", 15);
  run("serial-fib-20", 20);
  run("serial-fib-25", 25);
  run("serial-fib-30", 30);
  return 0;
}
