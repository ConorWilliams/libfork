#include "../bench.hpp"

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

#pragma omp task shared(a)
  a = fib(n - 1);

  b = fib(n - 2);

#pragma omp taskwait

  return a + b;
}

void run(std::string name, int x) {
  benchmark(name, [&](std::size_t n, auto&& bench) {
    int ans = 0;

#pragma omp parallel num_threads(n) shared(ans, x)
#pragma omp single nowait
    bench([&] {
      ans = fib(x);
    });

    return ans;
  });
}

auto main() -> int {
  run("omp, fib 5", 5);
  run("omp, fib 10", 10);
  run("omp, fib 15", 15);
  run("omp, fib 20", 20);
  run("omp, fib 25", 25);
  run("omp, fib 30", 30);
  return 0;
}
