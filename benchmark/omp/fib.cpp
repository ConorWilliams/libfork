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

auto serial_fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

  a = serial_fib(n - 1);
  b = serial_fib(n - 2);

  return a + b;
}

auto main() -> int {
  benchmark("omp, fib", [](std::size_t n, auto&& bench) {
    int ans = 0;

#pragma omp parallel num_threads(n)
#pragma omp single nowait
    bench([&] {
      ans = fib(25);
    });

    return ans;
  });

  return 0;
}