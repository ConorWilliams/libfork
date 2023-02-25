#include "../bench.hpp"

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

#pragma omp task untied shared(a)
  a = fib(n - 1);

  b = fib(n - 2);

#pragma omp taskwait

  return a + b;
}

void run(std::string name, int x) {
  benchmark(name, [&](std::size_t n, auto&& bench) {
    int answer = 0;

#pragma omp parallel num_threads(n) shared(answer, x)
#pragma omp single nowait
    bench([&] {
      answer = fib(x);
    });

    return answer;
  });
}

auto main() -> int {
  run("omp, fib 5", 5);
  run("omp, fib 10", 10);
  run("omp, fib 15", 15);
  run("omp, fib 20", 20);
  // run("omp, fib 25", 25); // too slow
  // run("omp, fib 30", 30);
  return 0;
}
