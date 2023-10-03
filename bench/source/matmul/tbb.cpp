#include <benchmark/benchmark.h>

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

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

void rec_matmul(double const *A, double const *B, double *C, int m, int n, int p, int ld) {

  if ((m + n + p) <= 64) {
    return multiply(A, B, C, m, n, p, ld);
  }

  tbb::task_group g;

  if (m >= n && n >= p) {
    int m1 = m >> 1;
    g.run([=]() {
      rec_matmul(A, B, C, m1, n, p, ld);
    });
    rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld);

  } else if (n >= m && n >= p) {
    int n1 = n >> 1;
    g.run([=]() {
      rec_matmul(A, B, C, m, n1, p, ld);
    });
    rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld);

  } else {
    int p1 = p >> 1;
    g.run([=]() {
      rec_matmul(A, B, C, m, n, p1, ld);
    });
    rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld);
  }
  g.wait();
}

void matmul_tbb(benchmark::State &state) {

  int num_threads = state.range(0);

  tbb::task_arena arena(num_threads);

  matmul_args args = arena.execute([] {
    return matmul_init(matmul_work);
  });

  for (auto _ : state) {
    state.PauseTiming();
    zero(args.C1.get(), args.n);
    state.ResumeTiming();

    arena.execute([&] {
      rec_matmul(args.A.get(), args.B.get(), args.C1.get(), args.n, args.n, args.n, args.n);
    });
  }

  auto &[A, B, C1, C2, n] = args;

  iter_matmul(A.get(), B.get(), C2.get(), n);

  if (maxerror(C1.get(), C2.get(), n) > 1e-6) {
    std::cout << "tbb maxerror: " << maxerror(C1.get(), C2.get(), n) << std::endl;
  }
}

} // namespace

BENCHMARK(matmul_tbb)->DenseRange(1, num_threads())->UseRealTime();
