#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "config.hpp"

namespace {

/*
 * A \in M(m, n)
 * B \in M(n, p)
 * C \in M(m, p)
 */
void rec_matmul(double const *A, double const *B, double *C, int m, int n, int p, int ld, bool add) {
  if ((m + n + p) <= 64) {
    if (add) {
      for (int i = 0; i < m; i++) {
        for (int k = 0; k < p; k++) {
          double c = 0.0;
          for (int j = 0; j < n; j++) {
            c += A[i * ld + j] * B[j * ld + k];
          }
          C[i * ld + k] += c;
        }
      }
    } else {
      for (int i = 0; i < m; i++) {
        for (int k = 0; k < p; k++) {
          double c = 0.0;
          for (int j = 0; j < n; j++) {
            c += A[i * ld + j] * B[j * ld + k];
          }
          C[i * ld + k] = c;
        }
      }
    }
  } else if (m >= n && n >= p) {
    int m1 = m >> 1;
    rec_matmul(A, B, C, m1, n, p, ld, add);
    rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, add);
  } else if (n >= m && n >= p) {
    int n1 = n >> 1;
    rec_matmul(A, B, C, m, n1, p, ld, add);
    rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, true);
  } else {
    int p1 = p >> 1;
    rec_matmul(A, B, C, m, n, p1, ld, add);
    rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
  }
}

void matmul_serial(benchmark::State &state) {

  lf::numa_topology{}.split(1).front().bind();

  auto [A, B, C1, C2, n] = matmul_init(1024);

  for (auto _ : state) {
    rec_matmul(A.get(), B.get(), C1.get(), n, n, n, n, false);
  }

  iter_matmul(A.get(), B.get(), C2.get(), n);

  if (maxerror(C1.get(), C2.get(), n) > 1e-6) {
    std::cout << "maxerror: " << maxerror(C1.get(), C2.get(), n) << std::endl;
  }
}

} // namespace

BENCHMARK(matmul_serial)->UseRealTime();
