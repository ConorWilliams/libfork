#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "config.hpp"

namespace {

void rec_matmul(double const *A, double const *B, double *C, int m, int n, int p, int ld) {
  if ((m + n + p) <= 64) {
    multiply(A, B, C, m, n, p, ld);
  } else if (m >= n && n >= p) {
    int m1 = m >> 1;
    rec_matmul(A, B, C, m1, n, p, ld);
    rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld);
  } else if (n >= m && n >= p) {
    int n1 = n >> 1;
    rec_matmul(A, B, C, m, n1, p, ld);
    rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld);
  } else {
    int p1 = p >> 1;
    rec_matmul(A, B, C, m, n, p1, ld);
    rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld);
  }
}

void matmul_serial(benchmark::State &state) {

  lf::numa_topology{}.split(1).front().bind();

  auto [A, B, C1, C2, n] = matmul_init(matmul_work);

  for (auto _ : state) {

    state.PauseTiming();
    zero(C1.get(), n);
    state.ResumeTiming();

    rec_matmul(A.get(), B.get(), C1.get(), n, n, n, n);
  }

  iter_matmul(A.get(), B.get(), C2.get(), n);

  if (maxerror(C1.get(), C2.get(), n) > 1e-6) {
    std::cout << "maxerror: " << maxerror(C1.get(), C2.get(), n) << std::endl;
  }
}

} // namespace

BENCHMARK(matmul_serial)->UseRealTime();
