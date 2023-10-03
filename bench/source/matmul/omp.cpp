#include <benchmark/benchmark.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void rec_matmul(double const *A, double const *B, double *C, int m, int n, int p, int ld) {
  if ((m + n + p) <= matmul_grain) {
    return multiply(A, B, C, m, n, p, ld);

  } else if (m >= n && n >= p) {
    int m1 = m >> 1;

#pragma omp task untied firstprivate(A, B, C, m1, n, p, ld) default(none)
    rec_matmul(A, B, C, m1, n, p, ld);
    rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld);
  } else if (n >= m && n >= p) {
    int n1 = n >> 1;
#pragma omp task untied firstprivate(A, B, C, m, n1, p, ld) default(none)
    rec_matmul(A, B, C, m, n1, p, ld);
    rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld);
  } else {
    int p1 = p >> 1;
#pragma omp task untied firstprivate(A, B, C, m, n, p1, ld) default(none)
    rec_matmul(A, B, C, m, n, p1, ld);
    rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld);
  }
#pragma omp taskwait
}

void matmul_omp(benchmark::State &state) {

  std::size_t n_thr = state.range(0);

  matmul_args args;

#pragma omp parallel num_threads(n_thr) shared(args)
#pragma omp single
  {
    args = matmul_init(matmul_work);

    auto *A = args.A.get();
    auto *B = args.B.get();
    auto *C1 = args.C1.get();
    auto *C2 = args.C2.get();
    auto n = args.n;

    for (auto _ : state) {
      state.PauseTiming();
      zero(C1, n);
      state.ResumeTiming();

      rec_matmul(A, B, C1, n, n, n, n);
    }

    iter_matmul(A, B, C2, n);

    if (maxerror(C1, C2, n) > 1e-6) {
      std::cout << "omp maxerror: " << maxerror(C1, C2, n) << std::endl;
    }
  }
}

} // namespace

BENCHMARK(matmul_omp)->DenseRange(1, num_threads())->UseRealTime();
